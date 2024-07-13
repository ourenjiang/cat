#include "BauCollector.h"
#include <iostream>
#include "utils/MsgpackWrapper_src.hpp"
#include <bitset>
#include "ems/base/BauWarning.h"
#include "utils/ModbusRtu.h"
#include "ems/base/HistoryWarning.h"
#include "ems/base/Modbus.h"

using namespace std;
using namespace ems;
using namespace ems::bau;

BauCollector::BauCollector(std::shared_ptr<zmq::socket_t> stationDealer, shared_ptr<SyncRequest> modbusProxy)
    : stationDealer_(stationDealer)
    , modbusProxy_(modbusProxy)
{
    stationDealer_->set(zmq::sockopt::rcvtimeo, 1000);
}

BauCollector::~BauCollector()
{
    if(loopThread_.joinable()) loopThread_.join();
}

void BauCollector::start()
{
    loopThread_ = std::thread([&]{
    while(true){
        doWork();
    }});
}

void BauCollector::doWork()
{
try
{
    zmq::message_t srcIdentity;
    auto result = stationDealer_->recv(srcIdentity);
    if(result.has_value()){
        // 接收到命令
        doCommand(srcIdentity);
        return;
    }

    // 执行轮询
    pollBingjiStatusFrame();
    pollBauStatusFrame();
    pollBcuFrame();
    pollBmuFrame();

    // 统计'可运行'、'可充电'、'可放电'的能力
    calcWorkAbility();

    // 向上发布数据
    auto serializedBody = msgpackWrapper::pack(bauInfo_);
    stationDealer_->send(zmq::message_t(string("Station")), zmq::send_flags::sndmore);
    stationDealer_->send(zmq::message_t(string("PublishInfo")), zmq::send_flags::sndmore);
    stationDealer_->send(zmq::message_t(string("BAU")), zmq::send_flags::sndmore);
    stationDealer_->send(zmq::message_t(string("0")), zmq::send_flags::sndmore);
    stationDealer_->send(zmq::message_t(serializedBody.data(), serializedBody.size()), zmq::send_flags::none);
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
}
}

optional<vector<byte>> BauCollector::sendAndRecv(const vector<uint8_t>& msg)
{
    auto msgptr = reinterpret_cast<const byte*>(msg.data());
    vector<byte> data(msgptr, msgptr + msg.size());
    
    const int64_t sndTimeoutMs{ 1000 };
    const int64_t rcvTimeoutMs{ 1000 };
    modbusProxy_->syncWrite(data, sndTimeoutMs);
    return modbusProxy_->syncRead(rcvTimeoutMs);
}

void BauCollector::pollBingjiStatusFrame()
{
    const uint16_t requestRegisterNum{ 0x0241 - 0x0200 + 1 };
    auto reqmsg = modbus::modbusRtuReadFrame(0x01, 0x03, 0x0200, requestRegisterNum);
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
    bauInfo_.bingjiStatusSummary = createBingjiStatusSummary(rawRegisters);
}

BingjiStatusSummary BauCollector::createBingjiStatusSummary(const vector<uint16_t>& frameRegisters)
{
    const uint16_t registerNum{ 0x42 };
    BOOST_ASSERT(frameRegisters.size() == registerNum);

    BingjiStatusSummary summary;
    summary.protectStatusL1 = modbus::getU32(0x0202, 0x0200, frameRegisters);
    summary.protectStatusL2 = modbus::getU32(0x0204, 0x0200, frameRegisters);
    summary.protectStatusL3 = modbus::getU32(0x0206, 0x0200, frameRegisters);
    summary.faultStatus = modbus::getU32(0x0208, 0x0200, frameRegisters);
    summary.specialStatus = modbus::getU32(0x0212, 0x0200, frameRegisters);
    return summary;
}

void BauCollector::pollBauStatusFrame()
{
    const uint16_t requestRegisterNum{ 0x0352 - 0x0300 + 1 };
    auto reqmsg = modbus::modbusRtuReadFrame(0x01, 0x03, 0x0300, requestRegisterNum);
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
    auto newSummary = createBauStatusSummary(rawRegisters);

    {
        // 计算统一的实时告警
        bauInfo_.warningStatus = calcBauUniformWarning(newSummary.protectStatusL1,
            newSummary.protectStatusL2, newSummary.protectStatusL3);
        // 告警数量
        {
            const bitset<25> bitParsed(newSummary.protectStatusL1);
            bauInfo_.warningL1Count = static_cast<int>(bitParsed.count());
        }
        {
            const bitset<25> bitParsed(newSummary.protectStatusL2);
            bauInfo_.warningL2Count = static_cast<int>(bitParsed.count());
        }
        {
            const bitset<25> bitParsed(newSummary.protectStatusL3);
            bauInfo_.warningL3Count = static_cast<int>(bitParsed.count());
        }
        {
            const bitset<3> bitParsed(newSummary.faultStatus);
            bauInfo_.faultCount = static_cast<int>(bitParsed.count());
        }
        // 是否存在告警或故障
        bauInfo_.existActiveWarningOrFault = (bauInfo_.warningL1Count > 0)
            || (bauInfo_.warningL2Count > 0) || (bauInfo_.warningL3Count > 0) || (bauInfo_.faultCount > 0);
        // 计算实时故障
        bauInfo_.faultStatus = calcBauFault(newSummary.faultStatus);
        // 计算系统状态
        bauInfo_.runStatus = calcBauRun(newSummary.systemStatus);
    }
    bauInfo_.bauStatusSummary = newSummary;

    // 检查BCU在线映射表是否发生改变，是则重置轮询位置.
    if(bcuPollMap_ != newSummary.bcuIndexListOnline){
        bcuPollMap_ = newSummary.bcuIndexListOnline;
        bcuPollItr_ = bcuPollMap_.begin();
        bmuPollMap_ = newSummary.bcuIndexListOnline;
        bmuPollItr_ = bmuPollMap_.begin();
    }
    if(bmuPollMap2_.empty()){
        for(int index = 0; index < newSummary.bcuSize; ++index){
            bmuPollMap2_.push_back(index);
        }
        bmuPollItr2_ = bmuPollMap2_.begin();
    }
}

BcuWarningStatus BauCollector::calcBcuUniformWarning(const uint32_t level1Bits, const uint32_t level2Bits, const uint32_t level3Bits)
{
    BcuWarningStatus status;
    using namespace base;
    status.cellVoltHigh                 = BauWarning::calcUniformLevel(0, level1Bits, level2Bits, level3Bits);
    status.cellVoltLow                  = BauWarning::calcUniformLevel(1, level1Bits, level2Bits, level3Bits);
    status.totalVoltHigh                = BauWarning::calcUniformLevel(2, level1Bits, level2Bits, level3Bits);
    status.totalVoltLow                 = BauWarning::calcUniformLevel(3, level1Bits, level2Bits, level3Bits);
    status.chargeOverCur                = BauWarning::calcUniformLevel(4, level1Bits, level2Bits, level3Bits);
    status.dischargeOverCur             = BauWarning::calcUniformLevel(5, level1Bits, level2Bits, level3Bits);
    status.chargeTemHigh                = BauWarning::calcUniformLevel(6, level1Bits, level2Bits, level3Bits);
    status.dischargeTemHigh             = BauWarning::calcUniformLevel(7, level1Bits, level2Bits, level3Bits);
    status.chargeTemLow                 = BauWarning::calcUniformLevel(8, level1Bits, level2Bits, level3Bits);
    status.dischargeTemLow              = BauWarning::calcUniformLevel(9, level1Bits, level2Bits, level3Bits);
    status.envTemHigh                   = BauWarning::calcUniformLevel(10, level1Bits, level2Bits, level3Bits);
    status.envTemLow                    = BauWarning::calcUniformLevel(11, level1Bits, level2Bits, level3Bits);
    status.chargeRelayTemHigh           = BauWarning::calcUniformLevel(12, level1Bits, level2Bits, level3Bits);
    status.dischargeRelayTemHigh        = BauWarning::calcUniformLevel(13, level1Bits, level2Bits, level3Bits);
    status.negativeRelayTemHigh         = BauWarning::calcUniformLevel(14, level1Bits, level2Bits, level3Bits);
    status.socHigh                      = BauWarning::calcUniformLevel(15, level1Bits, level2Bits, level3Bits);
    status.socLow                       = BauWarning::calcUniformLevel(16, level1Bits, level2Bits, level3Bits);
    status.positiveInsulationLeakage    = BauWarning::calcUniformLevel(17, level1Bits, level2Bits, level3Bits);
    status.negativeInsulationLeakage    = BauWarning::calcUniformLevel(18, level1Bits, level2Bits, level3Bits);
    status.chargeVoltDiff               = BauWarning::calcUniformLevel(19, level1Bits, level2Bits, level3Bits);
    status.dischargeVoltDiff            = BauWarning::calcUniformLevel(20, level1Bits, level2Bits, level3Bits);
    status.chargeTemDiff                = BauWarning::calcUniformLevel(21, level1Bits, level2Bits, level3Bits);
    status.dischargeTemDiff             = BauWarning::calcUniformLevel(22, level1Bits, level2Bits, level3Bits);

    status.cellTemIncrease              = BauWarning::calcUniformLevel(23, level1Bits, level2Bits, level3Bits);
    status.cellSampling                 = BauWarning::calcUniformLevel(24, level1Bits, level2Bits, level3Bits);
    status.ntcSamplingErr               = BauWarning::calcUniformLevel(25, level1Bits, level2Bits, level3Bits);
    status.terminalTemHigh              = BauWarning::calcUniformLevel(26, level1Bits, level2Bits, level3Bits);
    return status;
}

BcuFaultStatus BauCollector::calcBcuFault(const uint32_t faultBits)
{
    const bitset<17> bits(faultBits);

    BcuFaultStatus status;

    using namespace base;
    status.chargeRelayCombine       = bits[0];
    status.chargeRelayDisabled      = bits[1];
    status.dischargeRelayCombine    = bits[2];
    status.dischargeRelayDisabled   = bits[3];
    status.prechargeRelayCombine    = bits[4];
    status.prechargeRelayDisabled   = bits[5];
    status.negativeRelayCombine     = bits[6];
    status.negativeRelayDisabled    = bits[7];
    status.heatingFilmRelayCombine  = bits[8];
    status.heatingFileRelayDisabled = bits[9];
    status._12VErr                  = bits[10];
    status.cellFault                = bits[11];
    status.prechargeFault           = bits[12];
    status.heatingFilmFault         = bits[13];
    status.insulationBoardCommFault = bits[14];
    status.samplingBoardCommFault   = bits[15];
    status.curDiverterFault         = bits[16];
    status.ntcFault                 = bits[17];
    return status;
}

BcuRelayRealStatus BauCollector::calcBcuRelayRealStatus(const uint32_t faultBits)
{
    const bitset<5> bits(faultBits);

    BcuRelayRealStatus status;

    using namespace base;
    status.chargeRelay      = bits[0];
    status.dischargeRelay   = bits[1];
    status.prechargeRelay   = bits[2];
    status.negativeRelay    = bits[3];
    status.heatingFilmRelay = bits[4];
    return status;
}

BauWarningStatus BauCollector::calcBauUniformWarning(const uint32_t level1Bits, const uint32_t level2Bits, const uint32_t level3Bits)
{
    BauWarningStatus status;
    using namespace base;
    status.cellVoltHigh                 = BauWarning::calcUniformLevel(0, level1Bits, level2Bits, level3Bits);
    status.cellVoltLow                  = BauWarning::calcUniformLevel(1, level1Bits, level2Bits, level3Bits);
    status.totalVoltHigh                = BauWarning::calcUniformLevel(2, level1Bits, level2Bits, level3Bits);
    status.totalVoltLow                 = BauWarning::calcUniformLevel(3, level1Bits, level2Bits, level3Bits);
    status.chargeOverCur                = BauWarning::calcUniformLevel(4, level1Bits, level2Bits, level3Bits);
    status.dischargeOverCur             = BauWarning::calcUniformLevel(5, level1Bits, level2Bits, level3Bits);
    status.chargeTemHigh                = BauWarning::calcUniformLevel(6, level1Bits, level2Bits, level3Bits);
    status.dischargeTemHigh             = BauWarning::calcUniformLevel(7, level1Bits, level2Bits, level3Bits);
    status.chargeTemLow                 = BauWarning::calcUniformLevel(8, level1Bits, level2Bits, level3Bits);
    status.dischargeTemLow              = BauWarning::calcUniformLevel(9, level1Bits, level2Bits, level3Bits);
    status.envTemHigh                   = BauWarning::calcUniformLevel(10, level1Bits, level2Bits, level3Bits);
    status.envTemLow                    = BauWarning::calcUniformLevel(11, level1Bits, level2Bits, level3Bits);
    status.chargeRelayTemHigh           = BauWarning::calcUniformLevel(12, level1Bits, level2Bits, level3Bits);
    status.dischargeRelayTemHigh        = BauWarning::calcUniformLevel(13, level1Bits, level2Bits, level3Bits);
    status.negativeRelayTemHigh         = BauWarning::calcUniformLevel(14, level1Bits, level2Bits, level3Bits);
    status.socHigh                      = BauWarning::calcUniformLevel(15, level1Bits, level2Bits, level3Bits);
    status.socLow                       = BauWarning::calcUniformLevel(16, level1Bits, level2Bits, level3Bits);
    status.positiveInsulationLeakage    = BauWarning::calcUniformLevel(17, level1Bits, level2Bits, level3Bits);
    status.negativeInsulationLeakage    = BauWarning::calcUniformLevel(18, level1Bits, level2Bits, level3Bits);
    status.chargeVoltDiff               = BauWarning::calcUniformLevel(19, level1Bits, level2Bits, level3Bits);
    status.dischargeVoltDiff            = BauWarning::calcUniformLevel(20, level1Bits, level2Bits, level3Bits);
    status.chargeTemDiff                = BauWarning::calcUniformLevel(21, level1Bits, level2Bits, level3Bits);
    status.dischargeTemDiff             = BauWarning::calcUniformLevel(22, level1Bits, level2Bits, level3Bits);
    status.terminalTemHigh              = BauWarning::calcUniformLevel(23, level1Bits, level2Bits, level3Bits);
    status.bcuVoltDiff                  = BauWarning::calcUniformLevel(24, level1Bits, level2Bits, level3Bits);
    return status;
}

BauFaultStatus BauCollector::calcBauFault(const uint32_t faultBits)
{
    const bitset<5> bits(faultBits);

    BauFaultStatus status;

    using namespace base;
    status.canBusErr        = bits[0];
    status.rs485Err         = bits[1];
    status.bcuVersionErr    = bits[2];
    status.bcuAddrErr       = bits[3];
    status.bcuOfflineErr    = bits[4];
    return status;
}

BauRunStatus BauCollector::calcBauRun(const uint32_t faultBits)
{
    const bitset<6> bits(faultBits);

    BauRunStatus status;

    using namespace base;
    status.idle        = bits[0];
    status.bcuCodeing         = bits[1];
    status.insulationCheck    = bits[2];
    status.griding       = bits[3];
    status.charge    = bits[4];
    status.discharge    = bits[5];
    return status;
}

void BauCollector::pollBcuFrame()
{
    if(bcuPollMap_.empty())
        return;
    pollBcuFrame(*bcuPollItr_++);
    if(bcuPollItr_ == bcuPollMap_.end())
        bcuPollItr_ = bcuPollMap_.begin();
}

void BauCollector::pollBcuFrame(const int bcuIndex)
{
    const uint16_t bcuBeginAddr{ 0x0300 };
    const uint16_t bcuCapacity{ 0x0100 };
    const uint16_t bcuOffset{ static_cast<uint16_t>(bcuIndex * bcuCapacity) };

    const uint16_t requestRegisterNum{ 0x034A - 0x0300 + 1 };
    auto reqmsg = modbus::modbusRtuReadFrame(0x01, 0x04, bcuBeginAddr + bcuOffset, requestRegisterNum);
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
    auto newSummary = createBcuStatusSummary(rawRegisters);
    auto& bcuInfo = bauInfo_.bcuList[bcuIndex];
    {
        // 计算统一的实时告警
        bcuInfo.warningStatus = calcBcuUniformWarning(newSummary.protectStatusL1,
            newSummary.protectStatusL2, newSummary.protectStatusL3);
        // 告警数量
        {
            const bitset<25> bitParsed(newSummary.protectStatusL1);
            bcuInfo.warningL1Count = static_cast<int>(bitParsed.count());
        }
        {
            const bitset<25> bitParsed(newSummary.protectStatusL2);
            bcuInfo.warningL2Count = static_cast<int>(bitParsed.count());
        }
        {
            const bitset<25> bitParsed(newSummary.protectStatusL3);
            bcuInfo.warningL3Count = static_cast<int>(bitParsed.count());
        }
        {
            const bitset<3> bitParsed(newSummary.faultStatus);
            bcuInfo.faultCount = static_cast<int>(bitParsed.count());
        }
        // 是否存在告警或故障
        bcuInfo.existActiveWarningOrFault = (bcuInfo.warningL1Count > 0)
            || (bcuInfo.warningL2Count > 0) || (bcuInfo.warningL3Count > 0) || (bcuInfo.faultCount > 0);
        // 计算实时故障
        bcuInfo.faultStatus = calcBcuFault(newSummary.faultStatus);
        // 计算继电器真实状态
        bcuInfo.relayRealStatus = calcBcuRelayRealStatus(newSummary.relayRealStatus);
    }
    bcuInfo.base = newSummary;
}

void BauCollector::pollBmuFrame()
{
    if(bmuPollMap_.empty() || bmuPollMap2_.empty())
        return;
    pollBmuFrame(*bmuPollItr_, *bmuPollItr2_);
    if(++bmuPollItr2_ == bmuPollMap2_.end()){
        bmuPollItr2_ = bmuPollMap2_.begin();
        if(++bmuPollItr_ == bmuPollMap_.end()){
            bmuPollItr_ = bmuPollMap_.begin();
        }
    }
}

void BauCollector::pollBmuFrame(const int bcuIndx, const int bmuIndex)
{
    const int cellvoltSize = bauInfo_.bauStatusSummary.bmuCellVoltSize;
    const int cellTemSize = bauInfo_.bauStatusSummary.bmuCellTemSize;
    const int terminalTemSize = bauInfo_.bauStatusSummary.bmuTerminalTemSize;
    pollBmuCellvolt(bcuIndx, bmuIndex, cellvoltSize);
    pollBmuCelltem(bcuIndx, bmuIndex, cellTemSize, terminalTemSize);
}

void BauCollector::pollBmuCellvolt(const int bcuIndex, const int bmuIndex, const int cellvoltSize)
{
    const uint16_t cellvoltBeginAddr{ 0x0700 };
    const uint16_t bcuCapacity{ 20 };
    const uint16_t cellvoltCapacity{ 64 };
    const uint16_t bmuCapacity{ cellvoltCapacity };
    const uint16_t bcuOffset{ static_cast<uint16_t>(bcuIndex * bcuCapacity * bmuCapacity) };
    const uint16_t bmuOffset{ static_cast<uint16_t>(bmuIndex * bmuCapacity) };

    auto reqmsg = modbus::modbusRtuReadFrame(0x01, 0x03,
                                                cellvoltBeginAddr + bcuOffset + bmuOffset,
                                                cellvoltSize);

    auto repmsg = sendAndRecv(reqmsg);
    if(!repmsg.has_value())
        throw std::runtime_error("接收异常");
    auto repFrame = repmsg.value();

    // 解析帧
    const uint16_t rawRegistersNum = (repFrame.size() - 5) / sizeof(uint16_t);
    if(rawRegistersNum != cellvoltSize)
        throw std::logic_error("数据格式错误");
    vector<uint16_t> rawRegisters(rawRegistersNum);
    ::memcpy(rawRegisters.data(), repFrame.data() + 3, repFrame.size() - 5);
    // 转字节序
    vector<uint16_t> hostEndianRegisters;// 主机字节序
    for(auto itr = rawRegisters.begin(); itr != rawRegisters.end(); itr++){
        const uint16_t hostEndianData = be16toh(*itr);
        hostEndianRegisters.push_back(hostEndianData);
    }
    auto newSummary = createBmuCellvoltSummary(hostEndianRegisters);

    auto& bcuInfo = bauInfo_.bcuList[bcuIndex];
    auto& bmuList = bcuInfo.bmuList;
    auto& bmuInfo = bmuList[bmuIndex];
    bmuInfo.cellvoltInfo = newSummary;
}

void BauCollector::pollBmuCelltem(const int bcuIndex, const int bmuIndex, const int celltemSize, const int terminaltemSize)
{
    const uint16_t celltemBeginAddr{ 0x6C00 };
    const uint16_t bcuCapacity{ 20 };
    const uint16_t celltemCapacity{ 64 };
    const uint16_t terminaltemCapacity{ 2 };
    const uint16_t bmuCapacity{ celltemCapacity + terminaltemCapacity };
    const uint16_t bcuOffset{ static_cast<uint16_t>(bcuIndex * bcuCapacity * bmuCapacity) };
    const uint16_t bmuOffset{ static_cast<uint16_t>(bmuIndex * bmuCapacity) };

    auto reqmsg = modbus::modbusRtuReadFrame(0x01, 0x03,
                                            celltemBeginAddr + bcuOffset + bmuOffset,
                                            celltemCapacity + terminaltemSize);
    auto repmsg = sendAndRecv(reqmsg);
    if(!repmsg.has_value())
        throw std::runtime_error("接收异常");
    auto repFrame = repmsg.value();

    // 解析帧
    const uint16_t rawRegistersNum = (repFrame.size() - 5) / sizeof(uint16_t);
    if(rawRegistersNum != celltemCapacity + terminaltemSize)
        throw std::logic_error("数据格式错误");
    vector<uint16_t> rawRegisters(rawRegistersNum);
    ::memcpy(rawRegisters.data(), repFrame.data() + 3, repFrame.size() - 5);
    // 转字节序
    vector<uint16_t> hostEndianRegisters;// 主机字节序
    for(auto itr = rawRegisters.begin(); itr != rawRegisters.end(); itr++){
        const uint16_t hostEndianData = be16toh(*itr);
        hostEndianRegisters.push_back(hostEndianData);
    }

    // 截取 { 电芯温度数据块, 端子温度数据块 }
    std::vector<uint16_t> celltemData, terminaltemData;
    {
        auto& allTemData = hostEndianRegisters;
        std::move(allTemData.begin(), allTemData.begin() + celltemSize, back_inserter(celltemData));

        const uint16_t celltemCapacity{ 64 };
        std::move(allTemData.begin() + celltemCapacity, allTemData.end(),
                    std::back_inserter(terminaltemData));
    }
    auto newSummary = createBmuCelltemSummary(celltemData, terminaltemData);

    auto& bcuInfo = bauInfo_.bcuList[bcuIndex];
    auto& bmuList = bcuInfo.bmuList;
    auto& bmuInfo = bmuList[bmuIndex];
    bmuInfo.celltemInfo = newSummary;
}

CelltemSummary BauCollector::createBmuCelltemSummary(const vector<uint16_t>& cellTemRegisters, const vector<uint16_t>& terminalTemRegisters)
{
    CelltemSummary summary;

    auto transformTem = [this](vector<uint16_t>::const_iterator currentItr, vector<uint16_t>::const_iterator beginItr, vector<Tem>& dst){
        const auto index{ std::distance(beginItr, currentItr) + 1 };
        const uint16_t offset{ 2730 };
        const double rate{ 0.1 };
        const double value{ (*currentItr - offset) * rate };
        // BOOST_ASSERT(value > 0.0);
        dst.emplace_back(static_cast<uint16_t>(index), value);
    };

    auto& cellTemList{ summary.cellTemList };
    auto& terminalTemList{ summary.terminalTemList };
    for(auto itr = cellTemRegisters.begin(); itr != cellTemRegisters.end(); ++itr){
        transformTem(itr, cellTemRegisters.begin(), cellTemList);
    }
    for(auto itr = terminalTemRegisters.begin(); itr != terminalTemRegisters.end(); ++itr){
        transformTem(itr, terminalTemRegisters.begin(), terminalTemList);
    }
    {
        // 极大值
        auto pos = std::max_element(cellTemList.begin(), cellTemList.end(),
            [](const Tem& lhs, const Tem& rhs){
            return lhs.second < rhs.second;
        });
        BOOST_ASSERT(pos != cellTemList.end());
        summary.celltemMax = *pos;
    }
    {
        // 极小值
        auto pos = std::min_element(cellTemList.begin(), cellTemList.end(),
            [](const Tem& lhs, const Tem& rhs){
            return lhs.second < rhs.second;
        });
        BOOST_ASSERT(pos != cellTemList.end());
        summary.celltemMin = *pos;
    }
    {
        summary.temDiff = summary.celltemMax.second - summary.celltemMin.second;
    }
    return summary;
}

CellvoltSummary BauCollector::createBmuCellvoltSummary(const vector<uint16_t>& frameRegisters)
{
    CellvoltSummary summary;
    auto& cellvoltList = summary.cellvoltList;

    // step 1 : parse src data
    for(auto itr = frameRegisters.begin(); itr != frameRegisters.end(); ++itr){
        const auto index{ std::distance(frameRegisters.begin(), itr) + 1 };
        const double rate{ 0.001 };
        const double value{ *itr * rate };
        // BOOST_ASSERT(value > 0.0);
        cellvoltList.emplace_back(static_cast<uint16_t>(index), value);
    }
    {
        // 极大值
        auto pos = std::max_element(cellvoltList.begin(), cellvoltList.end(),
            [](const Volt& lhs, const Volt& rhs){
            return lhs.second < rhs.second;
        });
        BOOST_ASSERT(pos != cellvoltList.end());
        summary.cellvoltMax = *pos;
    }
    {
        // 极小值
        auto pos = std::min_element(cellvoltList.begin(), cellvoltList.end(),
            [](const Volt& lhs, const Volt& rhs){
            return lhs.second < rhs.second;
        });
        BOOST_ASSERT(pos != cellvoltList.end());
        summary.cellvoltMin = *pos;
    }
    {
        // 压差
        summary.voltDiff = summary.cellvoltMax.second - summary.cellvoltMin.second;
    }
    return summary;
}

void BauCollector::doCommand(zmq::message_t& srcIdentity)
{
    const string idString(static_cast<char*>(srcIdentity.data()), srcIdentity.size());
    zmq::message_t msg;
    (void)stationDealer_->recv(msg);
    const string msgString(static_cast<char*>(msg.data()), msg.size());

    vector<uint8_t> reqmsg(reinterpret_cast<const uint8_t*>(msg.data()),
                            reinterpret_cast<const uint8_t*>(msg.data()) + msg.size());
    auto repmsg = sendAndRecv(reqmsg);
    if(!repmsg.has_value()){
        pair<bool, vector<uint8_t>> respondMsg{true, {}};
        auto serializedBody = msgpackWrapper::pack(respondMsg);
        stationDealer_->send(srcIdentity, zmq::send_flags::sndmore);
        stationDealer_->send(zmq::message_t(serializedBody.data(), serializedBody.size()), zmq::send_flags::none);
        return;
    }
    auto repFrame = repmsg.value();
    vector<uint8_t> repFrame2(reinterpret_cast<const uint8_t*>(msg.data()),
                                reinterpret_cast<const uint8_t*>(msg.data()) + msg.size());
    pair<bool, vector<uint8_t>> respondMsg{true, repFrame2};
    auto serializedBody = msgpackWrapper::pack(respondMsg);
    stationDealer_->send(srcIdentity, zmq::send_flags::sndmore);
    stationDealer_->send(zmq::message_t(string("BAU0")), zmq::send_flags::sndmore);
    stationDealer_->send(zmq::message_t(serializedBody.data(), serializedBody.size()), zmq::send_flags::none);
}

BauStatusSummary BauCollector::createBauStatusSummary(const vector<uint16_t>& frameRegisters)
{
    const uint16_t registerNum{ 0x53 };
    BOOST_ASSERT(frameRegisters.size() == registerNum);

    BauStatusSummary summary;
    summary.systemStatus = modbus::getU16(0x0300, 0x0300, frameRegisters);
    summary.bcuBingjiNum = modbus::getU16(0x0301, 0x0300, frameRegisters);
    summary.protectStatusL1 = modbus::getU32(0x0302, 0x0300, frameRegisters);
    summary.protectStatusL2 = modbus::getU32(0x0304, 0x0300, frameRegisters);
    summary.protectStatusL3 = modbus::getU32(0x0306, 0x0300, frameRegisters);
    summary.faultStatus = modbus::getU32(0x0308, 0x0300, frameRegisters);

    {
        const uint16_t value{ modbus::getU16(0x0317, 0x0300, frameRegisters) };
        const double rate{ 0.1 };//100mV
        summary.volt = value * rate;
    }
    {
        const int32_t value{ modbus::getS32(0x0319, 0x0300, frameRegisters) };
        const double rate{ 0.01 };//10mA
        summary.cur = value * rate;
    }
    summary.soc = modbus::getU16(0x031B, 0x0300, frameRegisters);
    summary.soh = modbus::getU16(0x031C, 0x0300, frameRegisters);
    
    {
        const uint16_t value{ modbus::getU16(0x032D, 0x0300, frameRegisters) };
        const double rate{ 0.001 };//mv
        summary.cellvoltMax = value * rate;
    }
    // summary.cellvoltMaxAddr = getU16(0x032E, 0x0300, frameRegisters);
    summary.cellvoltMaxAddr = modbus::getU16(0x032C, 0x0300, frameRegisters);
    {
        const uint16_t value{ modbus::getU16(0x032F, 0x0300, frameRegisters) };
        const double rate{ 0.001 };//mv
        summary.cellvoltMin = value * rate;
    }
    summary.cellvoltMinAddr = modbus::getU16(0x032E, 0x0300, frameRegisters);
    {
        const uint16_t value{ modbus::getU16(0x0331, 0x0300, frameRegisters) };
        const double rate{ 0.1 };//0.1摄氏度
        summary.celltemMax = (value - 2730) * rate;
    }
    summary.celltemMaxAddr = modbus::getU16(0x0330, 0x0300, frameRegisters);
    {
        const uint16_t value{ modbus::getU16(0x0333, 0x0300, frameRegisters) };
        const double rate{ 0.1 };//0.1摄氏度
        summary.celltemMin = (value - 2730) * rate;
    }
    summary.celltemMinAddr = modbus::getU16(0x0332, 0x0300, frameRegisters);
    {
        const uint32_t value{ modbus::getU32(0x0334, 0x0300, frameRegisters) };
        const double rate{ 0.01 };
        summary.chargeCapacitySum = value * rate;
    }
    {
        const uint32_t value{ modbus::getU32(0x0336, 0x0300, frameRegisters) };
        const double rate{ 0.01 };
        summary.dischargeCapacitySum = value * rate;
    }
    {
        const uint16_t value{ modbus::getU16(0x0343, 0x0300, frameRegisters) };
        const double rate{ 0.1 };
        summary.pcsRequestChargeCur = value * rate;
    }
    {
        const uint16_t value{ modbus::getU16(0x0344, 0x0300, frameRegisters) };
        const double rate{ 0.1 };
        summary.pcsRequestChargeVolt = value * rate;
    }
    {
        const uint16_t value{ modbus::getU16(0x0345, 0x0300, frameRegisters) };
        const double rate{ 0.1 };
        summary.pcsRequestDischargeCur = value * rate;
    }
    {
        const uint16_t value{ modbus::getU16(0x0346, 0x0300, frameRegisters) };
        const double rate{ 0.1 };
        summary.pcsRequestDischargeVolt = value * rate;
    }
    {
        const uint32_t value{ modbus::getU32(0x0347, 0x0300, frameRegisters) };
        const double rate{ 0.01 };
        summary.allowChargeCapacity = value * rate;
    }
    {
        const uint32_t value{ modbus::getU32(0x0349, 0x0300, frameRegisters) };
        const double rate{ 0.01 };
        summary.allowDischargeCapacity = value * rate;
    }
    summary.bcuNum = modbus::getU16(0x034B, 0x0300, frameRegisters);
    summary.bcuOnlineNum = modbus::getU16(0x034C, 0x0300, frameRegisters);
    summary.bcuSize = modbus::getU16(0x034D, 0x0300, frameRegisters);
    summary.bmuCellVoltSize = modbus::getU16(0x034E, 0x0300, frameRegisters);
    summary.bmuCellTemSize = modbus::getU16(0x034F, 0x0300, frameRegisters);
    summary.bmuTerminalTemSize = modbus::getU16(0x0350, 0x0300, frameRegisters);

    {
        const uint32_t bcuIndexBitMap = modbus::getU32(0x0351, 0x0300, frameRegisters);
        const int bcuMaxSize{ 20 };
        const bitset<bcuMaxSize> bcuIndexBitSets(bcuIndexBitMap);
        for(int bcuIndex = 0; bcuIndex < bcuMaxSize; ++bcuIndex){
            if(bcuIndexBitSets[bcuIndex])
                summary.bcuIndexListOnline.push_back(bcuIndex);
        }
    }
    return summary;
}

BcuStatusSummary BauCollector::createBcuStatusSummary(const vector<uint16_t>& frameRegisters)
{
    const uint16_t registerNum{ 0x4B };
    BOOST_ASSERT(frameRegisters.size() == registerNum);

    BcuStatusSummary summary;
    summary.protectStatusL1 = modbus::getU32(0x0302, 0x0300, frameRegisters);
    summary.protectStatusL2 = modbus::getU32(0x0304, 0x0300, frameRegisters);
    summary.protectStatusL3 = modbus::getU32(0x0306, 0x0300, frameRegisters);
    summary.faultStatus = modbus::getU32(0x0308, 0x0300, frameRegisters);
    summary.relayRealStatus = modbus::getU32(0x030E, 0x0300, frameRegisters);

    {
        const uint16_t value{ modbus::getU16(0x0317, 0x0300, frameRegisters) };
        const double rate{ 0.1 };//100mV
        summary.volt = value * rate;
    }
    {
        const int32_t value{ modbus::getS32(0x0319, 0x0300, frameRegisters) };
        const double rate{ 0.01 };//10mA
        summary.cur = value * rate;
    }
    summary.soc = modbus::getU16(0x031B, 0x0300, frameRegisters);
    summary.soh = modbus::getU16(0x031C, 0x0300, frameRegisters);
    
    {
        const uint16_t value{ modbus::getU16(0x032D, 0x0300, frameRegisters) };
        const double rate{ 0.001 };//mv
        summary.cellvoltMax = value * rate;
    }
    summary.cellvoltMaxAddr = modbus::getU16(0x032C, 0x0300, frameRegisters);
    {
        const uint16_t value{ modbus::getU16(0x032F, 0x0300, frameRegisters) };
        const double rate{ 0.001 };//mv
        summary.cellvoltMin = value * rate;
    }
    summary.cellvoltMinAddr = modbus::getU16(0x032E, 0x0300, frameRegisters);
    {
        const uint16_t value{ modbus::getU16(0x0331, 0x0300, frameRegisters) };
        const double rate{ 0.1 };//0.1摄氏度
        summary.celltemMax = (value - 2730) * rate;
    }
    summary.celltemMaxAddr = modbus::getU16(0x0330, 0x0300, frameRegisters);
    {
        const uint16_t value{ modbus::getU16(0x0333, 0x0300, frameRegisters) };
        const double rate{ 0.1 };//0.1摄氏度
        summary.celltemMin = (value - 2730) * rate;
    }
    summary.celltemMinAddr = modbus::getU16(0x0332, 0x0300, frameRegisters);
    return summary;
}

void BauCollector::calcWorkAbility()
{
    const auto& bauStatusSummary = bauInfo_.bauStatusSummary;
    const auto& bingjiStatusSummary = bauInfo_.bingjiStatusSummary;
    const uint32_t bauThirdProtectStatus = bauStatusSummary.protectStatusL3;
    const uint32_t bauFaultStatus = bauStatusSummary.faultStatus;
    const uint32_t bingjiThirdProtectStatus = bingjiStatusSummary.protectStatusL3;
    const uint32_t bingjiFaultStatus = bingjiStatusSummary.faultStatus;
    const uint32_t bingjiSpecialStatus = bingjiStatusSummary.specialStatus;
    bauInfo_.allowRunning = allowRunning(bauThirdProtectStatus, bauFaultStatus, bingjiThirdProtectStatus, bingjiFaultStatus);
    bauInfo_.allowCharge = allowCharge(bauThirdProtectStatus, bingjiThirdProtectStatus, bingjiSpecialStatus);
    bauInfo_.allowDischarge = allowDischarge(bauThirdProtectStatus, bingjiThirdProtectStatus, bingjiSpecialStatus);
}

bool BauCollector::allowRunning(const uint32_t bauThirdProtectStatus, const uint32_t bauFaultStatus, const uint32_t bingjiThirdProtectStatus, const uint32_t bingjiFaultStatus)
{
    {
        /** 检查'BAU三级保护状态'中属于故障的状态：
         * 环境高温
         * 环境低温
         * 负极继电器高温
         * 正极绝缘漏电
         * 负极绝缘漏电
         * 端子高温
         * 簇间压差
        */
        const bitset<25> faultBits("00000000001100100110000110000000");
        if(bauThirdProtectStatus & faultBits.to_ulong()) return false;
    }
    {
        /** 检查'BAU故障状态'中属于故障的状态:
         * CAN总线异常
         * RS485异常
         * BCU版本异常
         * BCU地址异常
        */
        const bitset<5> faultBits("11110");
        if(bauFaultStatus & faultBits.to_ulong()) return false;
    }
    {
        /** 检查'BCU三级保护状态'中属于故障的状态:
         * 环境高温
         * 环境低温
         * 负极继电器高温
         * 正极绝缘漏电
         * 负极绝缘漏电
         * 电芯升温
         * 电芯采样
         * NTC采样异常
         * 端子高温
         */ 
        const bitset<25> faultBits("000000000011001001100001111");
        if(bingjiThirdProtectStatus & faultBits.to_ulong()) return false;
    }
    {
        /** 检查'BCU故障状态'中属于故障的状态:
         * 略。（所有）
        */
        const bitset<5> faultBits("111111111111111111111111111");
        if(bingjiFaultStatus & faultBits.to_ulong()) return false;
    }
    return true;
}

/*
检查是否有充电能力:
    注意，该动作应该在 verifyNormal()返回为真的条件之下，才有必要执行；
    先检查BAU这一级的三级保护状态中所有属于'充电故障'的标志位；
    再遍历BAU下面的所有BCU的三级保护状态中所有属于'充电故障'的标志位；
    以上所有均验证通过，才表示'有充电能力'
*/
bool BauCollector::allowCharge(const uint32_t bauThirdProtectStatus, const uint32_t bingjiThirdProtectStatus, const uint32_t specialStatus)
{
    {
        /** 检查'BAU三级保护状态'中属于禁充的状态:
         * 单体高压
         * 总体高压
         * 充电过流
         * 充电高温
         * 充电低温
         * 充电继电器高温
         * SOC高
         * 充电压差
         * 充电温差
         */
        const bitset<25> stopChargeBits("1010101010001001000101000");
        if(bauThirdProtectStatus & stopChargeBits.to_ulong()) return false;
    }
    {
        /** 检查'BCU三级保护状态'中属于禁充的状态:
         * 单体高压
         * 总体高压
         * 充电过流
         * 充电高温
         * 充电低温
         * 充电继电器高温
         * SOC高
         * 充电压差
         * 充电温差
        */
        const bitset<25> stopChargeBits("1010101010001001000101000");
        if(bingjiThirdProtectStatus & stopChargeBits.to_ulong()) return false;
    }
    {
        /** 检查'BCU特殊状态'中属于禁充的状态:
         * PCS禁充
        */
        const bitset<7> stopChargeBits("0000010");
        if(specialStatus & stopChargeBits.to_ulong()) return false;
    }
    return true;
}

/*
检查是否有放电能力:
    与检查充电能力类似，需要验证BAU及其下管理的所有BCU的状态，所有校验都通过才表示'有放电能力'
*/
bool BauCollector::allowDischarge(const uint32_t bauThirdProtectStatus, uint32_t bingjiThirdProtectStatus, const uint32_t specialStatus)
{
    {
        /** 检查'BAU三级保护状态'中属于禁放的状态:
         * 单体低压
         * 总体低压
         * 放电过流
         * 放电高温
         * 放电低温
         * 放电继电器高温
         * SOC低
         * 放电压差
         * 放电温差
         */
        const bitset<25> stopDischargeBits("0101010101000100100010100");
        if(bauThirdProtectStatus & stopDischargeBits.to_ulong()) return false;
    }
    {
        /** 检查'BCU三级保护状态'中属于禁放的状态:
         * 单体低压
         * 总体低压
         * 放电过流
         * 放电高温
         * 放电低温
         * 放电继电器高温
         * SOC低
         * 放电压差
         * 放电温差
        */
        const bitset<25> stopDischargeBits("0101010101000100100010100");
        if(bingjiThirdProtectStatus & stopDischargeBits.to_ulong()) return false;
    }
    {
        /** 检查'BCU特殊状态'中属于禁放的状态:
         * PCS禁放
        */
        const bitset<7> stopDischargeBits("0000001");
        if(specialStatus & stopDischargeBits.to_ulong()) return false;
    }
    return true;
}
