#include "PcsCollector.h"
#include <iostream>
#include <sstream>
#include <bitset>
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "utils/ModbusRtu.h"
#include "ems/base/Modbus.h"

using namespace std;
using namespace boost;
using namespace ems::pcs;

PcsCollector::PcsCollector(std::shared_ptr<zmq::socket_t> dataDealer, std::shared_ptr<zmq::socket_t> cmdDealer, std::shared_ptr<SyncRequest> modbusProxy)
    : dataDealer_(cmdDealer)
    , cmdDealer_(cmdDealer)
    , modbusProxy_(modbusProxy)
{
    cmdDealer_->set(zmq::sockopt::rcvtimeo, 1000);
}

PcsCollector::~PcsCollector()
{
    if(loopThread_.joinable()) loopThread_.join();
}

void PcsCollector::start()
{
    loopThread_ = std::thread([&]{
    while(true){
        doWork();
    }});
}

void PcsCollector::doWork()
{
try
{
    zmq::message_t srcIdentity;
    auto result = cmdDealer_->recv(srcIdentity);
    if(result.has_value()){
        // 接收到命令
        doCommand(srcIdentity);
        return;
    }

    doPoll_0406_0460();
    doPoll_0474_04D0();
    handlePcsFrame();

    // 向上发布数据
    auto serializedBody = msgpackWrapper::pack(pcsInfo_);
    dataDealer_->send(zmq::message_t(string("PublishInfo")), zmq::send_flags::sndmore);
    dataDealer_->send(zmq::message_t(string("PCS")), zmq::send_flags::sndmore);
    dataDealer_->send(zmq::message_t(string("0")), zmq::send_flags::sndmore);
    dataDealer_->send(zmq::message_t(serializedBody.data(), serializedBody.size()), zmq::send_flags::none);
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
}
}

void PcsCollector::doCommand(zmq::message_t& srcIdentity)
{
    zmq::message_t msg;
    (void)cmdDealer_->recv(msg);
    const string msgString(static_cast<char*>(msg.data()), msg.size());

    vector<uint8_t> reqmsg(reinterpret_cast<const uint8_t*>(msg.data()),
                            reinterpret_cast<const uint8_t*>(msg.data()) + msg.size());
    auto repmsg = sendAndRecv(reqmsg);
    if(!repmsg.has_value()){
        pair<bool, vector<uint8_t>> respondMsg{false, {}};
        auto serializedBody = msgpackWrapper::pack(respondMsg);
        cmdDealer_->send(srcIdentity, zmq::send_flags::sndmore);
        cmdDealer_->send(zmq::message_t(serializedBody.data(), serializedBody.size()), zmq::send_flags::none);
        return;
    }
    auto repFrame = repmsg.value();
    vector<uint8_t> repFrame2(reinterpret_cast<const uint8_t*>(msg.data()),
                                reinterpret_cast<const uint8_t*>(msg.data()) + msg.size());
    pair<bool, vector<uint8_t>> respondMsg{true, repFrame2};
    auto serializedBody = msgpackWrapper::pack(respondMsg);
    cmdDealer_->send(srcIdentity, zmq::send_flags::sndmore);
    cmdDealer_->send(zmq::message_t(serializedBody.data(), serializedBody.size()), zmq::send_flags::none);
}

void PcsCollector::doPoll_0406_0460()
{
    const uint16_t requestRegisterNum{ 0x0460 - 0x0406 + 1 };
    auto reqmsg = modbus::modbusRtuReadFrame(0x01, 0x03, 0x0406, requestRegisterNum);
    auto repmsg = sendAndRecv(reqmsg);
    if(!repmsg.has_value())
        throw std::runtime_error("接收异常");
    auto repFrame = repmsg.value();

    // 解析帧
    const uint16_t rawRegistersNum = (repFrame.size() - 5) / sizeof(uint16_t);
    if(rawRegistersNum != requestRegisterNum)
        throw std::logic_error("数据格式错误");
    vector<uint16_t> rawRegisters(rawRegistersNum);
    ::memcpy(rawRegisters.data(), repFrame.data() + 3, repFrame.size() - 5);
    pcsInfo_.frame_0406_0460_summary = createSummary_0406_0460(rawRegisters);
}

_0406_0460_Summary PcsCollector::createSummary_0406_0460(const vector<uint16_t>& frameRegisters)
{
    const uint16_t registerNum{ 0x0460 - 0x0406 + 1 };
    BOOST_ASSERT(frameRegisters.size() == registerNum);

    _0406_0460_Summary summary;
    summary.warnStatus1 = modbus::getU16(0x0406, 0x0406, frameRegisters);
    summary.warnStatus2 = modbus::getU16(0x0407, 0x0406, frameRegisters);
    summary.warnStatus3 = modbus::getU16(0x0408, 0x0406, frameRegisters);
    summary.switchStatus = modbus::getU16(0x0409, 0x0406, frameRegisters);
    summary.pcsStatus = modbus::getU16(0x040A, 0x0406, frameRegisters);
    summary.perihperalStatus = modbus::getU16(0x040B, 0x0406, frameRegisters);

    {
        const uint16_t value{ modbus::getU16(0x0456, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1V
        summary.gridLineVoltAB = value * rate;
    }
    {
        const uint16_t value{ modbus::getU16(0x0457, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1V
        summary.gridLineVoltBC = value * rate;
    }
    {
        const uint16_t value{ modbus::getU16(0x0458, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1V
        summary.gridLineVoltCA = value * rate;
    }
    {
        const int16_t value{ modbus::getS16(0x0459, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1A
        summary.gridCurA = value * rate;
    }
    {
        const int16_t value{ modbus::getS16(0x045A, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1A
        summary.gridCurB = value * rate;
    }
    {
        const int16_t value{ modbus::getS16(0x045B, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1A
        summary.gridCurC = value * rate;
    }
    return summary;
}

void PcsCollector::doPoll_0474_04D0()
{
    const uint16_t requestRegisterNum{ 0x04D0 - 0x0474 + 1 };
    auto reqmsg = modbus::modbusRtuReadFrame(0x01, 0x03, 0x0474, requestRegisterNum);
    auto repmsg = sendAndRecv(reqmsg);
    if(!repmsg.has_value())
        throw std::runtime_error("接收异常");
    auto repFrame = repmsg.value();

    // 解析帧
    const uint16_t rawRegistersNum = (repFrame.size() - 5) / sizeof(uint16_t);
    if(rawRegistersNum != requestRegisterNum)
        throw std::logic_error("数据格式错误");
    vector<uint16_t> rawRegisters(rawRegistersNum);
    ::memcpy(rawRegisters.data(), repFrame.data() + 3, repFrame.size() - 5);
    pcsInfo_.frame_0474_04D0_summary = createSummary_0474_04D0(rawRegisters);
}

_0474_04D0_Summary PcsCollector::createSummary_0474_04D0(const vector<uint16_t>& frameRegisters)
{
    const uint16_t registerNum{ 0x04D0 - 0x0474 + 1 };
    BOOST_ASSERT(frameRegisters.size() == registerNum);

    _0474_04D0_Summary summary;

    {
        const int16_t value{ modbus::getS16(0x04A6, 0x0474, frameRegisters) };
        const double rate{ 0.1 };//0.1V
        summary.activePowerSetting = value * rate;
    }
    {
        summary.remoteMode = modbus::getU16(0x04AB, 0x0474, frameRegisters);
    }
    {
        summary.powerSetting = modbus::getU16(0x04B3, 0x0474, frameRegisters);
    }
    return summary;
}

std::optional<vector<byte>> PcsCollector::sendAndRecv(const vector<uint8_t>& msg)
{
    auto msgptr = reinterpret_cast<const byte*>(msg.data());
    vector<byte> data(msgptr, msgptr + msg.size());
    
    const int64_t sndTimeoutMs{ 1000 };
    const int64_t rcvTimeoutMs{ 1000 };
    modbusProxy_->syncWrite(data, sndTimeoutMs);
    return modbusProxy_->syncRead(rcvTimeoutMs);
}

void PcsCollector::handlePcsFrame()
{
    auto& frame_0406_0460_summary = pcsInfo_.frame_0406_0460_summary;
    auto& frame_0474_04D0_summary = pcsInfo_.frame_0474_04D0_summary;
    pcsInfo_.warningStatus = calcWarning(frame_0406_0460_summary.warnStatus1, frame_0406_0460_summary.warnStatus2);
    pcsInfo_.faultStatus = calcFault(frame_0406_0460_summary.warnStatus1, frame_0406_0460_summary.warnStatus2);
    pcsInfo_.runStatus = calcRun(frame_0406_0460_summary.warnStatus3, frame_0406_0460_summary.switchStatus, frame_0406_0460_summary.pcsStatus);
    pcsInfo_.warningL1Count = 0;
    pcsInfo_.warningL2Count = 0;
    pcsInfo_.warningL3Count = calcWarningCount(pcsInfo_.warningStatus);
    pcsInfo_.faultCount = calcFaultCount(pcsInfo_.faultStatus);
    pcsInfo_.existActiveWarningOrFault = (pcsInfo_.warningL3Count > 0);
}

WarningStatus PcsCollector::calcWarning(const bitset<16> bits1, const bitset<16> bits2)
{
    WarningStatus status;
    status.warning1_invertOverCur = bits1[0];
    status.warning1_batteryVoltLow = bits1[3];
    status.warning1_batteryChargeDisabled = bits1[4];
    status.warning1_dcGeneratrixOverVolt = bits1[6];
    status.warning1_dcGeneratrixShortCircuit = bits1[7];
    status.warning1_outputContactorOpenCircuit = bits1[8];
    status.warning1_outputContactorShortCircuit = bits1[9];
    status.warning1_converterOverTem = bits1[10];
    status.warning1_outputOverLoad = bits1[11];

    status.warning2_gridOverVolt = bits2[0];
    status.warning2_gridLackVolt = bits2[1];
    status.warning2_gridPhaseOrderReverse = bits2[2];
    status.warning2_gridIslandingEffectProtect = bits2[4];
    status.warning2_batteryDischargeDisabled = bits2[8];
    status.warning2_energentPowerOff = bits2[13];
    status.warning2_converterNotSync = bits2[14];
    return status;
}

int PcsCollector::calcWarningCount(const WarningStatus status)
{
    int count{};
    if(status.warning1_invertOverCur) ++count;
    if(status.warning1_batteryVoltLow) ++count;
    if(status.warning1_batteryChargeDisabled) ++count;
    if(status.warning1_dcGeneratrixOverVolt) ++count;
    if(status.warning1_dcGeneratrixShortCircuit) ++count;
    if(status.warning1_outputContactorOpenCircuit) ++count;
    if(status.warning1_outputContactorShortCircuit) ++count;
    if(status.warning1_converterOverTem) ++count;
    if(status.warning1_outputOverLoad) ++count;
    if(status.warning2_gridOverVolt) ++count;
    if(status.warning2_gridLackVolt) ++count;
    if(status.warning2_gridPhaseOrderReverse) ++count;
    if(status.warning2_gridIslandingEffectProtect) ++count;
    if(status.warning2_batteryDischargeDisabled) ++count;
    if(status.warning2_energentPowerOff) ++count;
    if(status.warning2_converterNotSync) ++count;
    return count;
}

FaultStatus PcsCollector::calcFault(const bitset<16> bits1, const bitset<16> bits2)
{
    FaultStatus status;
    status.warning1_zbLimitCurFault = bits1[1];
    status.warning1_converterFault = bits1[2];
    status.warning1_bingjiCommFault = bits1[5];
    status.warning1_batteryConnectReverse = bits1[12];
    status.warning1_dcContactorFault = bits1[13];
    status.warning1_bmsCommFault = bits1[14];
    status.warning1_inverterLackPhaseFault = bits1[15];

    status.warning2_gridFrequencyErr = bits2[3];
    status.warning2_drivingLineFault = bits2[5];
    status.warning2_lightningProtectFault = bits2[6];
    status.warning2_insulationImpedanceErr = bits2[7];
    status.warning2_invertOverVoltFault = bits2[9];
    status.warning2_15VPowerFault = bits2[10];
    status.warning2_acFanFault = bits2[11];
    status.warning2_batteryFault = bits2[12];
    status.warning2_ctOrHallOpenCircuitFault = bits2[15];
    return status;
}

int PcsCollector::calcFaultCount(const FaultStatus status)
{
    int count{};
    if(status.warning1_zbLimitCurFault) ++count;
    if(status.warning1_converterFault) ++count;
    if(status.warning1_bingjiCommFault) ++count;
    if(status.warning1_batteryConnectReverse) ++count;
    if(status.warning1_dcContactorFault) ++count;
    if(status.warning1_bmsCommFault) ++count;
    if(status.warning1_inverterLackPhaseFault) ++count;
    if(status.warning2_gridFrequencyErr) ++count;
    if(status.warning2_drivingLineFault) ++count;
    if(status.warning2_lightningProtectFault) ++count;
    if(status.warning2_insulationImpedanceErr) ++count;
    if(status.warning2_invertOverVoltFault) ++count;
    if(status.warning2_15VPowerFault) ++count;
    if(status.warning2_acFanFault) ++count;
    if(status.warning2_batteryFault) ++count;
    if(status.warning2_ctOrHallOpenCircuitFault) ++count;
    return count;
}

RunStatus PcsCollector::calcRun(const uint16_t bits1, const uint16_t bits2, const uint16_t bits3)
{
    RunStatus status;
    const bitset<3> bits1Map(bits1);
    status.faultTotal = bits1Map[0];
    status.warnTotal = bits1Map[1];
    status.powerTotal = bits1Map[2];

    const bitset<6> bits2Map(bits2);
    status.dcInputBreaker = bits2Map[0];
    status.dcContactor = bits2Map[1];
    status.outputBreaker = bits2Map[2];
    status.outputContactor = bits2Map[3];
    status.gridBreaker = bits2Map[4];
    status.gridContactor = bits2Map[5];

    const bitset<3> bits3Map(bits3 >> 3);// 截取bit3 ~ bit5
    auto bit3_5 = bits3Map.to_ullong();// 变流器状态1
    status.gridOnCharge = (bit3_5 == 2);
    status.gridOnDischarge = (bit3_5 == 3);
    status.standby = (bit3_5 == 6);
    return status;
}
