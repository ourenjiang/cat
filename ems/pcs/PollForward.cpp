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
    zmqRespond_ = make_shared<ZmqRespond>(proxyAddress);

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
        const auto zmqRecvResult = zmqRespond_->recv();
        if(!zmqRecvResult.has_value()) continue;
        const auto recvmsg = zmqRecvResult.value();

        vector<uint8_t> requestMessage;
        const bool unserializedResult = msgpackWrapper::unpack(recvmsg.data(), recvmsg.size(), requestMessage);
        BOOST_ASSERT(unserializedResult);

        //消息代理
        const string frameStr(requestMessage.begin(), requestMessage.end());
        syncSocket_->asyncWrite(frameStr);
        const bool recvResult = syncSocket_->syncReadConditionVariable();
        if(!recvResult)
            continue;
        
        const string recvBuffer = syncSocket_->gerRecvBuffer();

        {
            vector<uint8_t> respondMessage(recvBuffer.begin(), recvBuffer.end());
            auto serializedMsg = msgpackWrapper::pack(respondMessage);// 序列化
            const vector<byte> sendmsg(reinterpret_cast<byte*>(serializedMsg.data()),
                                        reinterpret_cast<byte*>(serializedMsg.data()) + serializedMsg.size());
            const bool sendResult = zmqRespond_->send(sendmsg);
            if(!sendResult) continue;
        }
        // log_.debug("zmsg_recv success");
        log_.debug("pollMessage success");
    }});
}
