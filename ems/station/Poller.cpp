#include "Poller.h"
#include <iostream>
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems;

Poller::Poller()
{
}

void Poller::addSubscriber(const string& address, const vector<string>& topicList, 
                        function<void(const string&, const string&)> readCallback)
{
    auto& handler = handlers_.emplace_back(
        make_tuple<>(zmq::socket_t(zmq::socket_t(zmqContext_, zmq::socket_type::sub)),
        readCallback));

    auto& zmqSubscriber = std::get<0>(handler);
    for(const string& item: topicList){
        // zmqSubscriber.subscribe(item);
        zmqSubscriber.connect(address);
        zmqSubscriber.set(zmq::sockopt::subscribe, item);
    }

    // zmq::pollitem_t item{ zmqSubscriber.socket(), 0, ZMQ_POLLIN, 0 };
    zmq::pollitem_t item{ zmqSubscriber, 0, ZMQ_POLLIN, 0 };
    pollitems_.emplace_back(item);
}

void Poller::addRespondCallback(const string& key,
                            function<vector<byte> (std::shared_ptr<StationInfo>, const vector<byte>&)> callback)
{
    const string topic = miscellaneous::createFixedSizeString(key);
    const vector<byte> keyBytes(reinterpret_cast<const byte*>(topic.data()),
                            reinterpret_cast<const byte*>(topic.data()) + topic.size());
    respondCallbackMap_.emplace(keyBytes, callback);
}

void Poller::doPoll()
{
    if(!responser_)
    {
        responser_ = make_unique<ZmqRespond>("tcp://127.0.0.1:6200");
        zmq::pollitem_t item{ responser_->socket(), 0, ZMQ_POLLIN, 0 };
        pollitems_.emplace_back(item);
        responserPollIndex_ = pollitems_.size() - 1;
    }

    zmq::poll(pollitems_.data(), pollitems_.size(), std::chrono::seconds(3));

    /* handlers_ 与 pollitems_ 的数组长度保持一致；*/
    for(int index = 0; index < pollitems_.size(); ++index){

        const bool isReadable = static_cast<bool>(pollitems_[index].revents & ZMQ_POLLIN);
        if(!isReadable) continue;

        // 单独处理数据检查，后续还是得优化这块结构
        if(index == responserPollIndex_){
            doRespond();
            continue;
        }

        auto& [subscriber, callback] = handlers_[index];

        // string rBuffer;
        // const bool result = subscriber.recv(rBuffer);
        // callback(rBuffer);
        zmq::message_t topic;
        zmq::message_t body;
        (void)subscriber.recv(topic);
        (void)subscriber.recv(body);
        const string topicStream(static_cast<char*>(topic.data()), topic.size());
        const string bodyStream(static_cast<char*>(body.data()), body.size());
        callback(topicStream, bodyStream);
    }
}

void Poller::doRespond()
{
try
{
    const auto recvResult = responser_->recv();
    BOOST_ASSERT(recvResult.has_value());// 上层使用zmq::poll通知，所以这里一定可以接收到消息
    const auto recvmsg = recvResult.value();

    // 查找响应回调
    const vector<byte> topic{recvmsg.begin(), recvmsg.begin() + 64};
    const auto callbackItr = respondCallbackMap_.find(topic);
    BOOST_ASSERT(callbackItr != respondCallbackMap_.end());

    // 执行回调, 并获取响应内容
    const auto& respondCallback = callbackItr->second;
    const vector<byte> msgbody{ recvmsg.begin() + 64 , recvmsg.end()};
    const vector<byte> respondContent = respondCallback(stationInfo_, msgbody);
    // 发送响应内容
    const auto respondMessage = miscellaneous::createRespondMessage(true, respondContent);
    const bool sendResult = responser_->send(respondMessage);
    BOOST_ASSERT(sendResult);
}
catch(const std::exception& e){

    // 准备响应内容
    const string errmsg(e.what());
    const vector<byte> respondContent{ reinterpret_cast<const byte*>(errmsg.data()), 
                                        reinterpret_cast<const byte*>(errmsg.data()) + errmsg.size() };
    // 发送响应内容
    const auto msg = miscellaneous::createRespondMessage(false, respondContent);
    const bool sendResult = responser_->send(msg);
    BOOST_ASSERT(sendResult);
}
}
