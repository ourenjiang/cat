#include "Poller.h"
#include <iostream>
#include <sstream>
#include <bitset>
#include "boost/assert.hpp"
#include "utils/YamlcppWrapper.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace std;
using namespace boost;
using namespace ems::pcs;

Poller::Poller()
    : log_(ems::Log4cppWrapper::getLogger(4))
    , timer_(io_service_, asio::chrono::milliseconds(1000))
{
    {
        auto& cfgRoot = YamlcppWrapper::getRoot();
        const auto& collectors = cfgRoot["collector"];
        auto resultLoad = std::find_if(collectors.begin(), collectors.end(), [](const YAML::Node& item){
            return item["name"].as<string>() == "PCS";
        });
        BOOST_ASSERT(resultLoad != collectors.end());
        BOOST_ASSERT((*resultLoad)["load"].as<bool>());

        const string proxyAddress = (*resultLoad)["master"]["address"].as<string>();
        requester_ = make_unique<ZmqRequest>(proxyAddress);
    }
}

Poller::Poller(Poller&& other)
    : log_(other.log_)
    , timer_(std::move(other.timer_))
{
}

Poller::~Poller()
{
    if(loopThread_.joinable()) loopThread_.join();
}

void Poller::initPublishInterface(const std::string& ip, const uint16_t port)
{
    // 保存
    publishIp_ = ip ;
    publishPort_ = port;

    // 初始化发布接口
    publisher_ = make_unique<ZmqPublish>(ip + to_string(port));
}

void Poller::start()
{
    loopThread_ = thread([this]{
        timer_.async_wait(bind(&Poller::onTimeout, this, placeholders::_1));
        io_service_.run();// blocking
    });
}

void Poller::onTimeout(const system::error_code &error)
{
    doWork();
    // 重新注册定时器
    timer_.expires_from_now(asio::chrono::milliseconds(500));
    timer_.async_wait(bind(&Poller::onTimeout, this, placeholders::_1));
}

void Poller::doWork()
{
    auto obj_0406_0460_Summary = doPoll_0406_0460();
    auto obj_0474_04D0_Summary = doPoll_0474_04D0();

    if(obj_0406_0460_Summary.has_value() && obj_0474_04D0_Summary.has_value()){
        const string topic{ "PcsSummary" };// 主题名称
        this->publish(topic, branchIndex_, obj_0406_0460_Summary.value(), obj_0474_04D0_Summary.value());
    }
}

bool Poller::pollMessage(vector<uint8_t>& requestMessage, vector<uint8_t> &respondMessage)
{
    // 消息序列化
    auto serializedMsg = msgpackWrapper::pack(requestMessage);
    const vector<byte> sendmsg(reinterpret_cast<byte*>(serializedMsg.data()),
                                reinterpret_cast<byte*>(serializedMsg.data()) + serializedMsg.size());
    bool sendResult = requester_->send(sendmsg);
    if(!sendResult)
        log_.error("send failed");

    // 接收
    const auto recvResult = requester_->recv();
    if(!recvResult.has_value()){
        cout << "recv failed" << endl;
        return false;
    }
    const auto recvmsg = recvResult.value();

    // 2.2 消息反序列化
    if(!msgpackWrapper::unpack(recvmsg.data(), recvmsg.size(), respondMessage))
    {
        return false;
    }
    log_.debug("recv success");
    return true;
}

std::optional<_0406_0460_Summary> Poller::doPoll_0406_0460()
{
    auto rawMessage = miscellaneous::createModbusRtuReadFrame(0x01, 0x03, 0x0406, 0x0460 - 0x0406 + 1);

    vector<uint8_t> respondMessage;
    if(!pollMessage(rawMessage, respondMessage)) return {};
    if(respondMessage.empty())
    {
        log_.debug("未接收到有效数据");
        return {};
    }

    if(respondMessage.size() > 5){

        uint16_t* ptr = reinterpret_cast<uint16_t*>(respondMessage.data() + 3);
        vector<uint16_t> registers;
        for(int i = 0; i < (respondMessage.size() - 5) / sizeof(uint16_t); ++i){
            miscellaneous::reverseByteArray(ptr + i, sizeof(uint16_t));
            registers.push_back(*(ptr + i));
        }
        return createSummary_0406_0460(registers);
    }
    return {};
}

_0406_0460_Summary Poller::createSummary_0406_0460(const vector<uint16_t>& frameRegisters)
{
    const uint16_t registerNum{ 0x0460 - 0x0406 + 1 };
    BOOST_ASSERT(frameRegisters.size() == registerNum);

    _0406_0460_Summary summary;
    
    {
        summary.warnStatus1 = getU16(0x0406, 0x0406, frameRegisters);
        summary.warnStatus2 = getU16(0x0407, 0x0406, frameRegisters);
        summary.warnStatus3 = getU16(0x0408, 0x0406, frameRegisters);
        summary.switchStatus = getU16(0x0409, 0x0406, frameRegisters);
        summary.pcsStatus = getU16(0x040A, 0x0406, frameRegisters);
        summary.perihperalStatus = getU16(0x040B, 0x0406, frameRegisters);

        {
            // 告警状态字1
            const bitset<16> warnStatus1BitMap{ summary.warnStatus1 };
            const bitset<16> warnStatus2BitMap{ summary.warnStatus2 };

            summary.warnCountL1 = warnStatus1BitMap.count()
                                + warnStatus2BitMap.count();
        }
        {
            // 统计2级告警数量
            summary.warnCountL2 = 0;
        }
        {
            // 统计3级告警数量
            summary.warnCountL3 = 0;
        }
        summary.warnCountTotal = summary.warnCountL1 + summary.warnCountL2 + summary.warnCountL3;
    }

    {
        const uint16_t value{ getU16(0x0456, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1V
        summary.gridLineVoltAB = value * rate;
    }
    {
        const uint16_t value{ getU16(0x0457, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1V
        summary.gridLineVoltBC = value * rate;
    }
    {
        const uint16_t value{ getU16(0x0458, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1V
        summary.gridLineVoltCA = value * rate;
    }
    {
        const int16_t value{ getS16(0x0459, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1A
        summary.gridCurA = value * rate;
    }
    {
        const int16_t value{ getS16(0x045A, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1A
        summary.gridCurB = value * rate;
    }
    {
        const int16_t value{ getS16(0x045B, 0x0406, frameRegisters) };
        const double rate{ 0.1 };//0.1A
        summary.gridCurC = value * rate;
    }
    return summary;
}

std::optional<_0474_04D0_Summary> Poller::doPoll_0474_04D0()
{
    auto rawMessage = miscellaneous::createModbusRtuReadFrame(0x01, 0x03, 0x0474, 0x04D0 - 0x0474 + 1);

    vector<uint8_t> respondMessage;
    if(!pollMessage(rawMessage, respondMessage)) return {};
    if(respondMessage.empty()){
        log_.debug("未接收到有效数据");
        return {};
    }

    BOOST_ASSERT(respondMessage.size() > 5);
    BOOST_ASSERT((respondMessage.size() - 5) % 2 == 0);

    if(respondMessage.size() > 5){

        uint16_t* ptr = reinterpret_cast<uint16_t*>(respondMessage.data() + 3);
        vector<uint16_t> registers;
        for(int i = 0; i < (respondMessage.size() - 5) / sizeof(uint16_t); ++i){
            registers.push_back(*(ptr + i));
        }

        const string topic{ "Pcs_0474_04D0" };// 主题名称
        return createSummary_0474_04D0(registers);
    }
    return {};
}

_0474_04D0_Summary Poller::createSummary_0474_04D0(const vector<uint16_t>& frameRegisters)
{
    const uint16_t registerNum{ 0x04D0 - 0x0474 + 1 };
    BOOST_ASSERT(frameRegisters.size() == registerNum);

    _0474_04D0_Summary summary;

    {
        const int16_t value{ getS16(0x04A6, 0x0474, frameRegisters) };
        const double rate{ 0.1 };//0.1V
        summary.activePowerSetting = value * rate;
    }
    {
        summary.remoteMode = getU16(0x04AB, 0x0474, frameRegisters);
    }
    {
        summary.powerSetting = getU16(0x04B3, 0x0474, frameRegisters);
    }
    return summary;
}

void Poller::publish(const string& topic, const int location, const _0406_0460_Summary& obj_0406_0460_Summary, const _0474_04D0_Summary& obj_0474_04D0_Summary)
{
    vector<byte> msgBuffer(reinterpret_cast<const byte*>(topic.data()),
                                reinterpret_cast<const byte*>(topic.data() + topic.size()));
    auto msgBody = make_tuple(location, obj_0406_0460_Summary, obj_0474_04D0_Summary);

    auto serializedBody = msgpackWrapper::pack(msgBody);
    std::copy(reinterpret_cast<const byte*>(serializedBody.data()),
                reinterpret_cast<const byte*>(serializedBody.data() + serializedBody.size()), std::back_inserter(msgBuffer));

    if(!publisher_->send(msgBuffer.data(), msgBuffer.size()))
        log_.errorStream() << topic << " publish failed";
}

int16_t Poller::getS16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    const int offset{ addr - beginAddr };
    return static_cast<int16_t>(frameRegisters[offset]);
}

uint16_t Poller::getU16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters)
{
    const int offset{ addr - beginAddr };
    return frameRegisters[offset];
}
