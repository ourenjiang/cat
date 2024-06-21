#include "BauPoller.h"
#include <iostream>
#include <sstream>
#include <bitset>
#include "utils/YamlcppWrapper.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "boost/timer/timer.hpp"
#include "utils/Miscellaneous.h"
#include <endian.h>

using namespace std;
using namespace boost;
using namespace ems;
using namespace ems::bau;

BauPoller::BauPoller()
    : log_(Log4cppWrapper::getLogger(3))
    , timer_(io_service_, asio::chrono::milliseconds(500))
{
    {
        auto& cfgRoot = YamlcppWrapper::getRoot();
        const auto& collectors = cfgRoot["collector"];
        auto resultLoad = std::find_if(collectors.begin(), collectors.end(), [](const YAML::Node& item){
            return item["name"].as<string>() == "BAU";
        });
        BOOST_ASSERT(resultLoad != collectors.end());
        BOOST_ASSERT((*resultLoad)["load"].as<bool>());

        const string proxyAddress = (*resultLoad)["master"]["address"].as<string>();
        ZmqRequest_ = make_shared<ZmqRequest>(proxyAddress);
    }
}

BauPoller::BauPoller(BauPoller&& other)
    : log_(other.log_)
    , timer_(std::move(other.timer_))
{
}

void BauPoller::initPublishInterface(const std::string& ip, const uint16_t port)
{
    // 保存
    publishIp_ = ip ;
    publishPort_ = port;

    // 初始化发布接口
    publisher_ = make_shared<ZmqPublish>(ip + to_string(port));
}

BauPoller::~BauPoller()
{
    if(loopThread_.joinable()) loopThread_.join();
}

void BauPoller::start()
{
    loopThread_ = thread([this]{
        timer_.async_wait(bind(&BauPoller::onTimeout, this, placeholders::_1));
        io_service_.run();// blocking
    });
}

void BauPoller::onTimeout(const system::error_code &error)
{
    auto bingjiStatusSummary = doFrameBingjiStatus();
    auto bauStatusSummary = doFrameBauStatus();

    if(bingjiStatusSummary.has_value() && bauStatusSummary.has_value()){
        const string topic{ "BauSummary" };// 主题名称
        this->publish(topic, branchIndex_, bingjiStatusSummary.value(), bauStatusSummary.value());
    }

    // 重新注册定时器
    timer_.expires_from_now(asio::chrono::milliseconds(500));
    timer_.async_wait(bind(&BauPoller::onTimeout, this, placeholders::_1));
}

std::optional<vector<uint8_t>> BauPoller::pollMessage(vector<uint8_t>& reqmsg)
{
    // 序列化
    auto serializedMsg = msgpackWrapper::pack(reqmsg);
    // 发送
    const vector<byte> sendmsg(reinterpret_cast<byte*>(serializedMsg.data()),
                                reinterpret_cast<byte*>(serializedMsg.data()) + serializedMsg.size());
    bool sendResult = ZmqRequest_->send(sendmsg);
    if(!sendResult){
        log_.error("send failed");
        return {};
    }

    // 接收
    const auto recvResult = ZmqRequest_->recv();
    if(!recvResult.has_value()){
        log_.error("recv failed");
        return {};
    }

    // 反序列化
    const auto recvmsg = recvResult.value();
    vector<uint8_t> repmsg;
    if(!msgpackWrapper::unpack(recvmsg.data(), recvmsg.size(), repmsg)){
        return {};
    }
    return repmsg;
}

std::optional<BingjiStatusSummary> BauPoller::doFrameBingjiStatus()
{
try
{
    const uint16_t requestRegisterNum{ 0x0241 - 0x0200 + 1 };
    auto reqmsg = miscellaneous::createModbusRtuReadFrame(0x01, 0x03, 0x0200, requestRegisterNum);
    auto pollResult = pollMessage(reqmsg);
    if(!pollResult.has_value()){
        throw std::logic_error("未接收到有效数据");
    }
    const vector<uint8_t> repmsg = pollResult.value();

    uint16_t rawRegistersNum = (repmsg.size() - 5) / sizeof(uint16_t);
    if(rawRegistersNum != requestRegisterNum){
        throw std::logic_error("数据格式错误");
    }

    vector<uint16_t> rawRegisters(rawRegistersNum);
    ::memcpy(rawRegisters.data(), repmsg.data() + 3, repmsg.size() - 5);

    vector<uint16_t> bigEndianRegisters;
    for(auto itr = rawRegisters.begin(); itr != rawRegisters.end(); itr++){
        const uint16_t bigEndianData = htobe16(*itr);
        bigEndianRegisters.push_back(bigEndianData);
    }
    return createBingjiStatusSummary(bigEndianRegisters);
}
catch(const std::exception& e){
    log_.debugStream() << e.what();
}
    return {};
}

BingjiStatusSummary BauPoller::createBingjiStatusSummary(const vector<uint16_t>& frameRegisters)
{
    const uint16_t registerNum{ 0x42 };
    BOOST_ASSERT(frameRegisters.size() == registerNum);

    BingjiStatusSummary summary;

    {
        summary.protectStatusL1 = getU32(0x0202, 0x0200, frameRegisters);
        summary.protectStatusL2 = getU32(0x0204, 0x0200, frameRegisters);
        summary.protectStatusL3 = getU32(0x0206, 0x0200, frameRegisters);
        summary.faultStatus = getU32(0x0208, 0x0200, frameRegisters);

        {
            // 统计1级告警数量
            const bitset<32> bits{ summary.protectStatusL1 };
            const size_t totalCount{ bits.count() };
            summary.warnCountL1 = totalCount;
        }
        {
            // 统计2级告警数量
            const bitset<32> bits{ summary.protectStatusL2 };
            const size_t totalCount{ bits.count() };
            summary.warnCountL2 = totalCount;
        }
        {
            // 统计3级告警数量
            const bitset<32> protectStatusL2Bits{ summary.protectStatusL3 };
            const bitset<32> faultStatusBits{ summary.faultStatus };
            const size_t totalCount{ protectStatusL2Bits.count()
                                + faultStatusBits.count() };
            summary.warnCountL3 = totalCount;
        }
    }
    return summary;
}

std::optional<BauStatusSummary> BauPoller::doFrameBauStatus()
{
try
{
    // const uint16_t requestRegisterNum{ 0x0350 - 0x0300 + 1 };
    const uint16_t requestRegisterNum{ 0x0352 - 0x0300 + 1 };
    auto reqmsg = miscellaneous::createModbusRtuReadFrame(0x01, 0x03, 0x0300, requestRegisterNum);
    auto pollResult = pollMessage(reqmsg);
    if(!pollResult.has_value()){
        throw std::logic_error("未接收到有效数据");
    }
    const vector<uint8_t> repmsg = pollResult.value();

    uint16_t rawRegistersNum = (repmsg.size() - 5) / sizeof(uint16_t);
    if(rawRegistersNum != requestRegisterNum){
        throw std::logic_error("数据格式错误");
    }

    vector<uint16_t> rawRegisters(rawRegistersNum);
    ::memcpy(rawRegisters.data(), repmsg.data() + 3, repmsg.size() - 5);

    vector<uint16_t> bigEndianRegisters;
    for(auto itr = rawRegisters.begin(); itr != rawRegisters.end(); itr++){
        const uint16_t bigEndianData = htobe16(*itr);
        bigEndianRegisters.push_back(bigEndianData);
    }
    return createBauStatusSummary(bigEndianRegisters);
}
catch(const std::exception& e){
    log_.debugStream() << e.what();
}
    return {};
}

BauStatusSummary BauPoller::createBauStatusSummary(const vector<uint16_t>& frameRegisters)
{
    const uint16_t registerNum{ 0x53 };
    BOOST_ASSERT(frameRegisters.size() == registerNum);

    BauStatusSummary summary;
    
    {
        summary.protectStatusL1 = getU32(0x0302, 0x0300, frameRegisters);
        summary.protectStatusL2 = getU32(0x0304, 0x0300, frameRegisters);
        summary.protectStatusL3 = getU32(0x0306, 0x0300, frameRegisters);
        summary.faultStatus = getU32(0x0308, 0x0300, frameRegisters);

        {
            // 统计1级告警数量
            const bitset<32> bits{ summary.protectStatusL1 };
            const size_t totalCount{ bits.count() };
            summary.warnCountL1 = totalCount;
        }
        {
            // 统计2级告警数量
            const bitset<32> bits{ summary.protectStatusL2 };
            const size_t totalCount{ bits.count() };
            summary.warnCountL2 = totalCount;
        }
        {
            // 统计3级告警数量
            const bitset<32> protectStatusL2Bits{ summary.protectStatusL3 };
            const bitset<32> faultStatusBits{ summary.faultStatus };
            const size_t totalCount{ protectStatusL2Bits.count()
                                + faultStatusBits.count() };
            summary.warnCountL3 = totalCount;
        }
        summary.warnCountTotal = summary.warnCountL1 + summary.warnCountL2 + summary.warnCountL3;
    }

    summary.bcuBingjiNum = getU16(0x0301, 0x0300, frameRegisters);
    {
        const uint16_t value{ getU16(0x0317, 0x0300, frameRegisters) };
        const double rate{ 0.1 };//100mV
        summary.volt = value * rate;
    }
    {
        const int32_t value{ getS32(0x0319, 0x0300, frameRegisters) };
        const double rate{ 0.01 };//10mA
        summary.cur = value * rate;
    }
    summary.soc = getU16(0x031B, 0x0300, frameRegisters);
    summary.soh = getU16(0x031C, 0x0300, frameRegisters);
    
    {
        const uint16_t value{ getU16(0x032D, 0x0300, frameRegisters) };
        const double rate{ 0.001 };//mv
        summary.cellvoltMax = value * rate;
    }
    summary.cellvoltMaxAddr = getU16(0x032E, 0x0300, frameRegisters);
    {
        const uint16_t value{ getU16(0x032F, 0x0300, frameRegisters) };
        const double rate{ 0.001 };//mv
        summary.cellvoltMin = value * rate;
    }
    summary.cellvoltMinAddr = getU16(0x032E, 0x0300, frameRegisters);
    {
        const uint16_t value{ getU16(0x0331, 0x0300, frameRegisters) };
        const double rate{ 0.1 };//0.1摄氏度
        summary.celltemMax = (value - 2730) * rate;
    }
    summary.celltemMaxAddr = getU16(0x0330, 0x0300, frameRegisters);
    {
        const uint16_t value{ getU16(0x0333, 0x0300, frameRegisters) };
        const double rate{ 0.1 };//0.1摄氏度
        summary.celltemMin = (value - 2730) * rate;
    }
    summary.celltemMinAddr = getU16(0x0332, 0x0300, frameRegisters);
    {
        const uint32_t value{ getU32(0x0334, 0x0300, frameRegisters) };
        const double rate{ 0.01 };
        summary.chargeCapacitySum = value * rate;
    }
    {
        const uint32_t value{ getU32(0x0336, 0x0300, frameRegisters) };
        const double rate{ 0.01 };
        summary.dischargeCapacitySum = value * rate;
    }
    {
        const uint16_t value{ getU16(0x0343, 0x0300, frameRegisters) };
        const double rate{ 0.1 };
        summary.pcsRequestChargeCur = value * rate;
    }
    {
        const uint16_t value{ getU16(0x0344, 0x0300, frameRegisters) };
        const double rate{ 0.1 };
        summary.pcsRequestChargeVolt = value * rate;
    }
    {
        const uint16_t value{ getU16(0x0345, 0x0300, frameRegisters) };
        const double rate{ 0.1 };
        summary.pcsRequestDischargeCur = value * rate;
    }
    {
        const uint16_t value{ getU16(0x0346, 0x0300, frameRegisters) };
        const double rate{ 0.1 };
        summary.pcsRequestDischargeVolt = value * rate;
    }
    {
        const uint32_t value{ getU32(0x0347, 0x0300, frameRegisters) };
        const double rate{ 0.01 };
        summary.allowChargeCapacity = value * rate;
    }
    {
        const uint32_t value{ getU32(0x0349, 0x0300, frameRegisters) };
        const double rate{ 0.01 };
        summary.allowDischargeCapacity = value * rate;
    }
    summary.bcuNum = getU16(0x034B, 0x0300, frameRegisters);
    summary.bcuOnlineNum = getU16(0x034C, 0x0300, frameRegisters);
    summary.bcuSize = getU16(0x034D, 0x0300, frameRegisters);
    summary.bmuCellVoltSize = getU16(0x034E, 0x0300, frameRegisters);
    summary.bmuCellTemSize = getU16(0x034F, 0x0300, frameRegisters);
    summary.bmuTerminalTemSize = getU16(0x0350, 0x0300, frameRegisters);

    {
        const uint32_t bcuIndexBitMap = getU32(0x0351, 0x0300, frameRegisters);
        const int bcuMaxSize{ 20 };
        const bitset<bcuMaxSize> bcuIndexBitSets(bcuIndexBitMap);
        for(int bcuIndex = 0; bcuIndex < bcuMaxSize; ++bcuIndex){
            if(bcuIndexBitSets[bcuIndex])
                summary.bcuIndexListOnline.push_back(bcuIndex);
        }
    }
    
    {
        summary.cellNum = summary.bcuNum * summary.bcuSize;
    }
    return summary;
}

void BauPoller::publish(const string topic, const int location, const BingjiStatusSummary& bingjiStatusSummary, const BauStatusSummary& bauStatusSummary)
{
    vector<byte> msgBuffer(reinterpret_cast<const byte*>(topic.data()),
                                reinterpret_cast<const byte*>(topic.data() + topic.size()));
    auto msgBody = make_tuple(location, bingjiStatusSummary, bauStatusSummary);

    auto serializedBody = msgpackWrapper::pack(msgBody);
    std::copy(reinterpret_cast<const byte*>(serializedBody.data()),
                reinterpret_cast<const byte*>(serializedBody.data() + serializedBody.size()), std::back_inserter(msgBuffer));

    if(!publisher_->send(msgBuffer.data(), msgBuffer.size()))
        log_.errorStream() << topic << " publish failed";
}

uint16_t BauPoller::getU16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    const int offset{ addr - beginAddr };
    return frameRegisters[offset];
}

uint32_t BauPoller::getU32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    const int offset{ addr - beginAddr };
    const int highOffset{ offset }, lowOffset{ offset + 1 };
    return (frameRegisters[highOffset] << 16) + frameRegisters[lowOffset];
}

int32_t BauPoller::getS32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    const int offset{ addr - beginAddr };
    const uint32_t highBigValue{ static_cast<uint32_t>(frameRegisters[offset] << 16) };
    const uint16_t lowBitValue{ frameRegisters[offset + 1] };
    return static_cast<int32_t>(highBigValue + lowBitValue);
}

