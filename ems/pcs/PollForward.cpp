#include "PollForward.h"
#include <filesystem> // since c++17
#include "utils/YamlcppWrapper.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "boost/assert.hpp"

using namespace ems;
using namespace ems::pcs;

PollForward::PollForward()
    : log_(ems::Log4cppWrapper::getLogger(2))
    , zmqRouter_(zmqContext_, zmq::socket_type::router)
{
    // 启动代理服务
    auto& cfgRoot = YamlcppWrapper::getRoot();
    const auto& collectors = cfgRoot["collector"];
    auto resultLoadPcs = std::find_if(collectors.begin(), collectors.end(), [](const YAML::Node& item){
        bool isBau = item["name"].as<string>() == "PCS";
        bool isLoad = item["load"].as<bool>();
        return isBau && isLoad;
    });
    BOOST_ASSERT(resultLoadPcs != collectors.end());
    const string proxyAddress = (*resultLoadPcs)["proxy"]["address"].as<string>();
    zmqRouter_.bind(proxyAddress);

    // 创建modbus连接
    string ip, port;
    const string slaveAddress = (*resultLoadPcs)["slave"]["address"].as<string>();
    miscellaneous::parseIpPortString(slaveAddress, ip, port);
    syncSocket_ = make_shared<SyncSocketRequest>(ip, port);
}

PollForward::~PollForward()
{
    if(loopThread_.joinable())
    {
        loopThread_.join();
    }
}

void PollForward::start()
{
    loopThread_ = thread([this]{
    while(true)
    {
        zmq::message_t identity;
        (void)zmqRouter_.recv(identity);
        zmq::message_t delimiter;
        (void)zmqRouter_.recv(delimiter);
        zmq::message_t rcvmsg;
        (void)zmqRouter_.recv(rcvmsg);

        //消息代理
        auto pollResult = pollModbusSlave(rcvmsg);
        if(pollResult.has_value()){

            const auto& modbusRespond = pollResult.value();
            // auto serializedMsg = msgpackWrapper::pack(modbusRespond);
            // zmq::message_t sndmsg(serializedMsg.data(), serializedMsg.size());
            zmq::message_t sndmsg(modbusRespond.data(), modbusRespond.size());
            zmqRouter_.send(identity, zmq::send_flags::sndmore);
            zmqRouter_.send(delimiter, zmq::send_flags::sndmore);
            zmqRouter_.send(sndmsg, zmq::send_flags::none);
            log_.debug("pollMessage success");
        }
        else{
            log_.debug("pollMessage failed");
        }
    }});
}

optional<vector<byte>> PollForward::pollModbusSlave(const zmq::message_t& msg)
{
    vector<uint8_t> requestMessage(reinterpret_cast<const uint8_t*>(msg.data()),
        reinterpret_cast<const uint8_t*>(msg.data()) + msg.size());

    //转发
    syncSocket_->asyncWrite({ reinterpret_cast<const char*>(requestMessage.data()), requestMessage.size() });
    const bool syncSocketRecvResult = syncSocket_->syncReadConditionVariable();
    if(syncSocketRecvResult){
        const string msg = syncSocket_->gerRecvBuffer();
        return vector<byte>{ reinterpret_cast<const byte*>(msg.data()), reinterpret_cast<const byte*>(msg.data()) + msg.size() };
    }
    return {};
}