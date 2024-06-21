#include "PollForward.h"
#include <filesystem> // since c++17
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/YamlcppWrapper.h"
#include "boost/assert.hpp"

using namespace std;
using namespace ems::bau;

PollForward::PollForward()
    : log_(ems::Log4cppWrapper::getLogger(1))
{
    // 启动代理服务
    auto& cfgRoot = YamlcppWrapper::getRoot();
    const auto& collectors = cfgRoot["collector"];
    auto resultLoadBau = std::find_if(collectors.begin(), collectors.end(), [](const YAML::Node& item){
        bool isBau = item["name"].as<string>() == "BAU";
        bool isLoad = item["load"].as<bool>();
        return isBau && isLoad;
    });
    BOOST_ASSERT(resultLoadBau != collectors.end());
    const string proxyAddress = (*resultLoadBau)["proxy"]["address"].as<string>();
    zmqRespond_ = make_shared<ZmqRespond>(proxyAddress);

    // 创建'同步请求'客户端
    string ip, port;
    const string slaveAddress = (*resultLoadBau)["slave"]["address"].as<string>();
    miscellaneous::parseIpPortString(slaveAddress, ip, port);
    syncSocket_ = make_shared<SyncSocketRequest>(ip, port);
}

PollForward::~PollForward()
{
    if(loopThread_.joinable()) loopThread_.join();
}

void PollForward::start()
{
    loopThread_ = thread([this]{
    while(true)
    {
        const auto zmqRecvResult = zmqRespond_->recv();
        if(!zmqRecvResult.has_value()) continue;
        const auto recvmsg = zmqRecvResult.value();
        
        vector<uint8_t> requestMessage;
        const bool unserializedResult = msgpackWrapper::unpack(recvmsg.data(), recvmsg.size(), requestMessage);
        BOOST_ASSERT(unserializedResult);

        //消息代理
        const string frameStr(requestMessage.begin(), requestMessage.end());
        syncSocket_->asyncWrite(frameStr);
        const bool syncSocketRecvResult = syncSocket_->syncReadConditionVariable();
        if(!syncSocketRecvResult) continue;
        
        const string recvBuffer = syncSocket_->gerRecvBuffer();

        {
            vector<uint8_t> respondMessage(recvBuffer.begin(), recvBuffer.end());
            auto rbuffer = msgpackWrapper::pack(respondMessage);// 序列化
            const vector<byte> sendmsg(reinterpret_cast<byte*>(rbuffer.data()),
                                        reinterpret_cast<byte*>(rbuffer.data()) + rbuffer.size());
            const bool sendResult = zmqRespond_->send(sendmsg);
            if(!sendResult) continue;
        }
        // log_.debug("zmsg_recv success");
        log_.debug("pollMessage success");
    }});
}
