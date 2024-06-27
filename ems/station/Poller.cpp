#include "Poller.h"
#include <iostream>
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems;

Poller::Poller()
{
    {
        auto& router = zmqSockets_.emplace_back(miscellaneous::createZmqSocket(zmq::socket_type::router));
        router.bind("tcp://127.0.0.1:6200");

        zmq::pollitem_t item{ router, 0, ZMQ_POLLIN, 0 };
        pollitems_.emplace_back(item);

        using namespace std::placeholders;
        pollerCallbacks_.emplace_back(std::bind(&Poller::doRespond, this, _1, _2, _3, _4, _5));
    }
}

void Poller::addSubscriber(const string& address, const vector<string>& topicList, PollerCallback callback)
{
    auto& subscriber = zmqSockets_.emplace_back(miscellaneous::createZmqSocket(zmq::socket_type::sub));
    subscriber.connect(address);
    for(const string& item: topicList){
        subscriber.set(zmq::sockopt::subscribe, item);
    }

    zmq::pollitem_t item{ subscriber, 0, ZMQ_POLLIN, 0 };
    pollitems_.emplace_back(item);

    pollerCallbacks_.emplace_back(callback);
}

void Poller::addRespondCallback(const vector<byte>& identity, RespondCallback callback)
{
    // const string topic = miscellaneous::createFixedSizeString(key);
    // const vector<byte> keyBytes(reinterpret_cast<const byte*>(topic.data()),
    //                         reinterpret_cast<const byte*>(topic.data()) + topic.size());
    // respondCallbackMap_.emplace(keyBytes, callback);
    BOOST_ASSERT(!identity.empty());
    BOOST_ASSERT(callback);
    BOOST_ASSERT(respondCallbacks_.find(identity) == respondCallbacks_.end());

    respondCallbacks_[identity] = callback;
}

void Poller::doPoll()
{
    zmq::poll(pollitems_.data(), pollitems_.size(), std::chrono::seconds(3));

    /* handlers_ 与 pollitems_ 的数组长度保持一致；*/
    for(int index = 0; index < pollitems_.size(); ++index){

        const bool isReadable = static_cast<bool>(pollitems_[index].revents & ZMQ_POLLIN);
        if(!isReadable) continue;

        BOOST_ASSERT(index < zmqSockets_.size());
        BOOST_ASSERT(zmqSockets_.size() == pollitems_.size());
        BOOST_ASSERT(zmqSockets_.size() == pollerCallbacks_.size());

        auto& activeSocket = zmqSockets_[index];
        zmq::message_t topicOrIdentity;
        zmq::message_t subtitle;
        zmq::message_t body;
        (void)activeSocket.recv(topicOrIdentity);
        (void)activeSocket.recv(subtitle);
        (void)activeSocket.recv(body);
        
        const string identityString(reinterpret_cast<char*>(topicOrIdentity.data()), 
                reinterpret_cast<char*>(topicOrIdentity.data()) + topicOrIdentity.size());

        auto& callback = pollerCallbacks_[index];

        const vector<byte> topicOrIdentityStream(reinterpret_cast<byte*>(topicOrIdentity.data()),
                            reinterpret_cast<byte*>(topicOrIdentity.data()) + topicOrIdentity.size());
        const vector<byte> subtitleStream(reinterpret_cast<byte*>(subtitle.data()),
                            reinterpret_cast<byte*>(subtitle.data()) + subtitle.size());
        const vector<byte> bodyStream(reinterpret_cast<byte*>(body.data()),
                            reinterpret_cast<byte*>(body.data()) + body.size());
        callback(stationInfo_, activeSocket, topicOrIdentityStream, subtitleStream, bodyStream);
    }
}

void Poller::doRespond(std::shared_ptr<StationInfo>& stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body)
{
    const string identityString(reinterpret_cast<const char*>(identity.data()),
        reinterpret_cast<const char*>(identity.data()) + identity.size());

    const auto callbackItr = respondCallbacks_.find(identity);
    BOOST_ASSERT(callbackItr != respondCallbacks_.end());

    // 执行回调
    const auto& respondCallback = callbackItr->second;
    respondCallback(stationInfo_, router, identity, subtitle, body);
}
