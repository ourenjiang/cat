#include "StrategyProtect.h"
#include <iostream>
#include <filesystem> // since c++17
#include <regex>
#include "boost/assert.hpp"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/crc16.h"
#include "utils/datetime.h"
#include "utils/endian.h"
#include "ems/base/StationInfo.h"
#include "utils/ModbusRtu.h"

using namespace std;
using namespace std::chrono;
using namespace boost;
using namespace ems;
using namespace ems::xftg;

StrategyProtect::StrategyProtect(std::shared_ptr<zmq::socket_t> stationDealer)
    : log_(ems::Log4cppWrapper::getLogger(0))
    , stationDealer_(stationDealer)
{
    stationDealer_->set(zmq::sockopt::rcvtimeo, 1000);
}

StrategyProtect::~StrategyProtect()
{
    if(loopThread_.joinable()){
        loopThread_.join();
    }
}

void StrategyProtect::start()
{
    loopThread_ = std::thread([&]{
    while(true){
        doWork();
    }});
}

void StrategyProtect::control(const StationInfo& stationInfo)
{
    const auto& bauInfo = stationInfo.bauMap_.at(0);
    const auto& pcsInfo = stationInfo.pcsMap_.at(0);
    doBauUpdateInfo(bauInfo);

    if(!params.pcsCurrentStatus.powerTotal)
        data = getFramePowerOn();// 本轮先开机，下轮再执行正常工作
    /////////////////////////////////////////////////////////////////

    // 请求
    stationDealer_->send(zmq::message_t(), zmq::send_flags::sndmore);// Topic
    stationDealer_->send(zmq::message_t(string("PCS0")), zmq::send_flags::sndmore);// devId
    stationDealer_->send(zmq::message_t(string("Strategy0")), zmq::send_flags::sndmore);// return id
    // stationDealer_->send(zmq::message_t(data.data(), data.size()), zmq::send_flags::none);

    if(params.pcsCurrentStatus.powerTotal)// 告警、故障产生，需要关机
            data = getFramePowerOff();
}

void StrategyProtect::doWork()
{
try
{
    zmq::message_t srcIdentity;
    auto result = stationDealer_->recv(srcIdentity);
    if(!result.has_value()){

        // 请求站点更新数据
        stationDealer_->send(zmq::message_t(string("Station")), zmq::send_flags::sndmore);
        stationDealer_->send(zmq::message_t(string("ReadInfo")), zmq::send_flags::none);

        return;
    }

    const string idString(static_cast<char*>(srcIdentity.data()), srcIdentity.size());
    if(idString == "Station"){// 来自站点的‘采集数据’返回
        zmq::message_t rcvmsg;
        (void)stationDealer_->recv(rcvmsg);
        const string msgString(static_cast<char*>(rcvmsg.data()), rcvmsg.size());

        StationInfo stationInfo;
        const bool unpackResult = msgpackWrapper::unpack(rcvmsg.data(), rcvmsg.size(), stationInfo);
        BOOST_ASSERT(unpackResult);

        // 基于最新数据进行控制
        control(stationInfo);
    }
    else if(idString == "PCS0"){// 来自PCS的控制返回,不需要返回

    }
    else if(idString == "BAU0"){// 来自PCS的控制返回,不需要返回
        zmq::message_t rcvmsg;
        (void)stationDealer_->recv(rcvmsg);

        // 解析消息
        pair<bool, vector<uint8_t>> respondMsg;
        const bool unpackMsgResult = msgpackWrapper::unpack(rcvmsg.data(), rcvmsg.size(), respondMsg);
        BOOST_ASSERT(unpackMsgResult);
        const auto& [returnStatus, returnContent] = respondMsg;
        if(!returnStatus)
            throw std::runtime_error("modbus respond failed");
    }
    else if(idString == "Interface"){

        // 接收用户接口控制，需要返回
        // doCommand(srcIdentity);
    }
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
}
}

vector<uint8_t> StrategyProtect::getFramePowerOn()
{
    vector<uint8_t> requestMessage(8);
    {
        requestMessage[0] = 0x01;
        requestMessage[1] = 0x06;
        {
            uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestMessage[2]);
            *registerAddrPtr = 0x04CC;//开关机方式
            endian::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
        }
        {
            uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestMessage[4]);
            *registerDataPtr = 0x5555;// 开机
            endian::reverseByteArray(registerDataPtr, sizeof(uint16_t));
        }
        {
            uint16_t crc16Modbus = modbus::crc16_manual(requestMessage.data(), 6);
            std::memcpy(&requestMessage[6], &crc16Modbus, sizeof(uint16_t));
        }
    }
    return requestMessage;
}

vector<uint8_t> StrategyProtect::getFramePowerOff()
{
    vector<uint8_t> requestMessage(8);
    {
        requestMessage[0] = 0x01;
        requestMessage[1] = 0x06;
        {
            uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestMessage[2]);
            *registerAddrPtr = 0x04CC;//开关机方式
            endian::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
        }
        {
            uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestMessage[4]);
            *registerDataPtr = 0xAAAA;// 关机
            endian::reverseByteArray(registerDataPtr, sizeof(uint16_t));
        }
        {
            uint16_t crc16Modbus = modbus::crc16_manual(requestMessage.data(), 6);
            std::memcpy(&requestMessage[6], &crc16Modbus, sizeof(uint16_t));
        }
    }
    return requestMessage;
}

bool StrategyProtect::doBauUpdateInfo(const bau::BauInfo& bauInfo)
{
    const auto& bau = bauInfo;

    const uint32_t batteryBauThirdProtectStatus = bau.bauStatusSummary.protectStatusL3;
    const uint32_t batteryBauFaultStatus = bau.bauStatusSummary.faultStatus;
    const uint32_t batteryBingjiThirdProtectStatus = bau.bingjiStatusSummary.protectStatusL3;
    const uint32_t batteryBingjiFaultStatus = bau.bingjiStatusSummary.faultStatus;

    vector<uint8_t> data;

    // 告警、故障位检查
    return bauStatus_.verifyNormal(batteryBauThirdProtectStatus, batteryBauFaultStatus,
                                    batteryBingjiThirdProtectStatus, batteryBingjiFaultStatus);
}

bool StrategyProtect::doPcsUpdateInfo(const pcs::PcsInfo& pcsInfo)
{
    const auto& pcs = pcsInfo;

    const double pcsCurrentSettingPower = pcs.frame_0474_04D0_summary.activePowerSetting;
    const double pcsCurrentOutputPower = pcs.frame_0474_04D0_summary.activePowerSetting;
    const pcs::RunStatus pcsCurrentStatus = pcs.runStatus;

    return true;
}
