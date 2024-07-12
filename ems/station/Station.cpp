#include "Station.h"
#include <iostream>
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/xftg/DayPlanDuration.h"
#include "utils/Miscellaneous.h"
#include <type_traits>
#include "utils/datetime.h"

using namespace ems;

Station::Station()
{
    zmqRouter_ = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::router);
    zmqRouter_->bind("tcp://*:9005");
}

Station::~Station()
{
    if(loopThread_.joinable()) loopThread_.join();
}

void Station::doPublishInfo()
{
    zmq::message_t devName; // 设备名称
    zmq::message_t devIndex;// 设备序号
    zmq::message_t body;    // 正文
    (void)zmqRouter_->recv(devName);
    (void)zmqRouter_->recv(devIndex);
    (void)zmqRouter_->recv(body);

    const string nameString(static_cast<char*>(devName.data()), devName.size());
    const string indexString(static_cast<char*>(devIndex.data()), devIndex.size());

    if(nameString == "BAU"){
        bau::BauInfo bauInfo;
        const bool unpackResult = msgpackWrapper::unpack(body.data(), body.size(), bauInfo);
        BOOST_ASSERT(unpackResult);
        cout << "bau parse success" << endl;
        const int index = std::stoi(indexString);
        stationInfo_.bauMap_[index] = bauInfo;
        return;
    }
    if(nameString == "PCS"){
        pcs::PcsInfo pcsInfo;
        const bool unpackResult = msgpackWrapper::unpack(body.data(), body.size(), pcsInfo);
        BOOST_ASSERT(unpackResult);
        cout << "pcs parse success" << endl;
        const int index = std::stoi(indexString);
        stationInfo_.pcsMap_[index] = pcsInfo;
        return;
    }
    if(nameString == "Strategy"){
        xftg::StrategyInfo strategyInfo;
        const bool unpackResult = msgpackWrapper::unpack(body.data(), body.size(), strategyInfo);
        BOOST_ASSERT(unpackResult);
        cout << "strategy parse success" << endl;
        const int index = std::stoi(indexString);
        stationInfo_.strategyMap_[index] = strategyInfo;
        return;
    }
}

void Station::doReadInfo(zmq::message_t& identity)
{
    auto serializedBody = msgpackWrapper::pack(stationInfo_);
    zmq::message_t reqmsg(serializedBody.data(), serializedBody.size());
    zmqRouter_->send(identity, zmq::send_flags::sndmore);
    zmqRouter_->send(zmq::message_t(string("Station")), zmq::send_flags::sndmore);
    zmqRouter_->send(reqmsg, zmq::send_flags::none);
}

void Station::doWork()
{
try
{
    // 接收ID
    zmq::message_t identity;
    auto result = zmqRouter_->recv(identity);
    ////////////////////////////////////////////////////////

    zmq::message_t target;
    (void)zmqRouter_->recv(target);
    const string targetString(static_cast<char*>(target.data()), target.size());
    if(targetString == "Station"){
        // 接收一级主题
        zmq::message_t topic;
        (void)zmqRouter_->recv(topic);
        const string topicString(static_cast<char*>(topic.data()), topic.size());
        if(topicString == "PublishInfo"){
            doPublishInfo();
        }
        else if(topicString == "ReadInfo"){
            doReadInfo(identity);
        }
    }
    else{
        zmqRouter_->send(target, zmq::send_flags::sndmore);// 二级路由地址
        while(true){
            zmq::message_t msg;
            (void)zmqRouter_->recv(msg);
            const string msgstr(static_cast<char*>(msg.data()), static_cast<char*>(msg.data()) + msg.size());
            const int isRcvmore = zmqRouter_->get(zmq::sockopt::rcvmore);
            zmqRouter_->send(msg, isRcvmore ? zmq::send_flags::sndmore : zmq::send_flags::none);
            if(!isRcvmore) break;
        }
    }
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
}
}

void Station::start()
{
    loopThread_ = std::thread([&]{
    while(true){
        doWork();
    }});
}
