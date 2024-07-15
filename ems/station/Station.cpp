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
    dataRouter_ = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::router);
    dataRouter_->bind("tcp://*:9005");

    cmdRouter_ = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::router);
    cmdRouter_->bind("tcp://*:9006");
}

Station::~Station()
{
    if(dataThread_.joinable()) dataThread_.join();
    if(cmdThread_.joinable()) cmdThread_.join();
}

void Station::start()
{
    dataThread_ = std::thread([&]{
    while(true){
        doData();
    }});

    cmdThread_ = std::thread([&]{
    while(true){
        doCommand();
    }});
}

void Station::doData()
{
try
{
    // 接收ID
    zmq::message_t identity;
    auto result = dataRouter_->recv(identity);
    ////////////////////////////////////////////////////////

    // 接收一级主题
    zmq::message_t topic;
    (void)dataRouter_->recv(topic);
    const string topicString(static_cast<char*>(topic.data()), topic.size());
    if(topicString == "PublishInfo"){
        doPublishInfo();
    }
    else if(topicString == "ReadInfo"){
        doReadInfo(identity);
    }
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
}
}

void Station::doCommand()
{
try
{
    // 接收ID
    zmq::message_t identity;
    auto result = dataRouter_->recv(identity);
    ////////////////////////////////////////////////////////

    /**
     * 注意，这里转发消息时，需要剥离消息源的路由地址，
     * 执行转发的消息是从第2帧开始，且第2帧是由消息源指定的定向路由地址;
    */

    while(true){
        zmq::message_t msg;
        (void)cmdRouter_->recv(msg);
        const string msgstr(static_cast<char*>(msg.data()), static_cast<char*>(msg.data()) + msg.size());
        const int isRcvmore = cmdRouter_->get(zmq::sockopt::rcvmore);
        cmdRouter_->send(msg, isRcvmore ? zmq::send_flags::sndmore : zmq::send_flags::none);
        if(!isRcvmore) break;
    }
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
}
}

void Station::doPublishInfo()
{
    zmq::message_t devName; // 设备名称
    zmq::message_t devIndex;// 设备序号
    zmq::message_t body;    // 正文
    (void)dataRouter_->recv(devName);
    (void)dataRouter_->recv(devIndex);
    (void)dataRouter_->recv(body);

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
    dataRouter_->send(identity, zmq::send_flags::sndmore);
    // dataRouter_->send(zmq::message_t(string("Station")), zmq::send_flags::sndmore);
    dataRouter_->send(reqmsg, zmq::send_flags::none);
}
