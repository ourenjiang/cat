#include "BcuPoller.h"
#include <iostream>
#include <sstream>
#include <bitset>
#include "utils/YamlcppWrapper.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "boost/timer/timer.hpp"
#include "utils/Miscellaneous.h"

using namespace std;
using namespace boost;
using namespace ems::bau;

BcuPoller::BcuPoller()
    : log_(ems::Log4cppWrapper::getLogger(3))
    , bcuIndexListOnlineItr_(bcuIndexListOnline_.begin())
    , zmqDealer_(zmqContext_, zmq::socket_type::dealer)
{
    auto& cfgRoot = YamlcppWrapper::getRoot();
    const auto& collectors = cfgRoot["collector"];
    auto resultLoad = std::find_if(collectors.begin(), collectors.end(), [](const YAML::Node& item){
        return item["name"].as<string>() == "BAU";
    });
    BOOST_ASSERT(resultLoad != collectors.end());
    BOOST_ASSERT((*resultLoad)["load"].as<bool>());

    const string proxyAddress = (*resultLoad)["master"]["address"].as<string>();
    zmqDealer_.connect(proxyAddress);
}

BcuPoller::BcuPoller(BcuPoller&& other)
    : log_(other.log_)
{
}

BcuPoller::~BcuPoller()
{
    if(loopThread_.joinable()) loopThread_.join();
}

void BcuPoller::initPublishInterface(const std::string& addr)
{
    // 初始化发布接口
    bcuFramePublisher_ = make_shared<ems::ZmqPublish>(addr);
}

void BcuPoller::start()
{
    loopThread_ = std::thread([this]{
    while(true){
        string rBuffer;
        bool recvResult = bauFrameSubscriber_->recv(rBuffer);
        if(!recvResult){
            /* 接收失败时，不需要对套接字进行重置处理，继续尝试接收. */
            continue;
        }

        this_thread::sleep_for(chrono::milliseconds(100));// 错峰
        const bool result = catchFrameBauBauStatus(rBuffer);
        BOOST_ASSERT(result);
        
        const uint16_t bcuNum{ bauStatus_.bcuOnlineNum };
        if(bcuNum == 0) return;

        // if(pollerCurrentBcuIndex_ >= bcuNum) pollerCurrentBcuIndex_ = 0;
        // doBcu(pollerCurrentBcuIndex_++);// prepare next
        if(bcuIndexListOnline_.empty()) continue;
        if(bcuIndexListOnlineItr_ == bcuIndexListOnline_.end())
            bcuIndexListOnlineItr_ = bcuIndexListOnline_.begin();
        doBcu(*bcuIndexListOnlineItr_++);// prepare next
        bauStatus_.bcuIndexListOnline;
    }});
}

void BcuPoller::subscribeBauStatus(const std::string& addr, const std::string& topic)
{
    bauFrameSubscriber_ = make_shared<ZmqSubscribe>(addr);
    bauFrameSubscriber_->subscribe(topic);
}

bool BcuPoller::catchFrameBauBauStatus(const string& message)
{
    const string topic{ "BauSummary" };
    if(message.compare(0, topic.size(), topic) != 0){
        return false;
    }

    tuple<int, bau::BingjiStatusSummary, bau::BauStatusSummary> requestBody;
    const bool unpackResult = msgpackWrapper::unpack(message.data() + topic.size(),
                                                message.size() - topic.size(), 
                                                requestBody);
    BOOST_ASSERT(unpackResult);

    auto& [branchIndex, bingjiStatusSummaryNew, bauStatusSummaryNew] = requestBody;
    if(bcuIndexListOnline_ != bauStatusSummaryNew.bcuIndexListOnline){
        bcuIndexListOnline_ = bauStatusSummaryNew.bcuIndexListOnline;
        bcuIndexListOnlineItr_ = bcuIndexListOnline_.begin();
    }
    bauStatus_ = bauStatusSummaryNew;
    return true;
}

std::optional<vector<uint8_t>> BcuPoller::pollMessage(const vector<uint8_t>& reqmsg)
{
    // // 消息序列化
    // auto serializedMsg = msgpackWrapper::pack(requestMessage);
    // const vector<byte> sendmsg(reinterpret_cast<byte*>(serializedMsg.data()),
    //                             reinterpret_cast<byte*>(serializedMsg.data()) + serializedMsg.size());
    {
        // zmq::message_t sndmsg(serializedMsg.data(), serializedMsg.size());
        zmq::message_t sndmsg(reqmsg.data(), reqmsg.size());

        zmq::message_t delimiter;
        zmqDealer_.send(delimiter, zmq::send_flags::sndmore);
        zmqDealer_.send(sndmsg, zmq::send_flags::none);
    }

    {
        zmq::message_t delimiter;
        zmqDealer_.recv(delimiter);

        zmq::message_t rcvmsg;
        zmqDealer_.recv(rcvmsg);

        // vector<uint8_t> repmsg;
        // msgpackWrapper::unpack(rcvmsg.data(), rcvmsg.size(), repmsg);
        const vector<uint8_t> repmsg(reinterpret_cast<uint8_t*>(rcvmsg.data()),
                                        reinterpret_cast<uint8_t*>(rcvmsg.data()) + rcvmsg.size());
        return repmsg;
    }
    return {};
}

void BcuPoller::doBcu(const uint16_t bcuIndex)
{
    const uint16_t bcuBeginAddr{ 0x0300 };
    const uint16_t bcuCapacity{ 0x0100 };
    const uint16_t bcuOffset{ static_cast<uint16_t>(bcuIndex * bcuCapacity) };

    auto rawMessage = miscellaneous::createModbusRtuReadFrame(0x01, 0x04, 
                                            bcuBeginAddr + bcuOffset,
                                            0x034A - 0x0300 + 1);

    auto pollResult = pollMessage(rawMessage);
    if(!pollResult.has_value()) return;

    auto& msg = pollResult.value();
    BOOST_ASSERT(msg.size() > 5);
    BOOST_ASSERT((msg.size() - 5) % 2 == 0);

    vector<uint16_t> registers;
    for(size_t i = 3; i < msg.size() - 2; i+= 2){
        const uint16_t value = static_cast<uint16_t>(msg[i] << 8 | msg[i+1]);
        registers.push_back(value);
    }
    BOOST_ASSERT(registers.size() == 0x034A - 0x0300 + 1);

    const string topic{ "BcuStatus" };// 主题名称
    tuple<int, int> location{ branchIndex_, bcuIndex };
    auto summary = createBcuStatusSummary(registers);
    {
        string msg;
        std::copy(topic.data(), topic.data() + topic.size(), std::back_inserter(msg));

        auto msgBody = tuple_cat(location, tie(summary));
        auto serializedBody = msgpackWrapper::pack(msgBody);
        std::copy(serializedBody.data(), serializedBody.data() + serializedBody.size(), std::back_inserter(msg));

        if(!bcuFramePublisher_->send(msg.data(), msg.size()))
            log_.debugStream() << topic << " publish failed";
    }
}

BcuStatusSummary BcuPoller::createBcuStatusSummary(const vector<uint16_t>& frameRegisters)
{
    const uint16_t registerNum{ 0x4B };
    BOOST_ASSERT(frameRegisters.size() == registerNum);

    BcuStatusSummary summary;

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
    summary.cellvoltMaxAddr = getU16(0x032C, 0x0300, frameRegisters);
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
        summary.gridStatus = false;
        const std::bitset<32> relayRealStatus(getU32(0x030E, 0x0300, frameRegisters));// 继电器真实状态
        const bool chargeRelay{ relayRealStatus[0] };// 充电继电器
        const bool dischargeRelay{ relayRealStatus[1] };// 放电继电器
        if(chargeRelay || dischargeRelay){
            summary.gridStatus = true;
        }
    }
    return summary;
}

uint16_t BcuPoller::getU16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    const int offset{ addr - beginAddr };
    return frameRegisters[offset];
}

uint32_t BcuPoller::getU32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    const int offset{ addr - beginAddr };
    const int highOffset{ offset }, lowOffset{ offset + 1 };
    return (frameRegisters[highOffset] << 16) + frameRegisters[lowOffset];
}

int32_t BcuPoller::getS32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    const int offset{ addr - beginAddr };
    const uint32_t highBigValue{ static_cast<uint32_t>(frameRegisters[offset] << 16) };
    const uint16_t lowBitValue{ frameRegisters[offset + 1] };
    return static_cast<int32_t>(highBigValue + lowBitValue);
}