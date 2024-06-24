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
    , zmqDealer_(zmqContext_, zmq::socket_type::dealer)
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
        zmqDealer_.connect(proxyAddress);
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

std::optional<vector<uint8_t>> Poller::pollMessage(const vector<uint8_t>& reqmsg)
{
    {
        zmq::message_t delimiter;
        zmqDealer_.send(delimiter, zmq::send_flags::sndmore);
        zmq::message_t sndmsg(reqmsg.data(), reqmsg.size());
        zmqDealer_.send(sndmsg, zmq::send_flags::none);
    }
    
    zmq::pollitem_t item{ zmqDealer_, 0, ZMQ_POLLIN, 0 };
    const int pollResult = zmq::poll(&item, 1, std::chrono::seconds(1));
    BOOST_ASSERT(pollResult == 0 || pollResult == 1);
    if(pollResult == 0){
        log_.errorStream() << "recv timeout";
        return {};
    }

    zmq::message_t delimiter;
    (void)zmqDealer_.recv(delimiter);
    zmq::message_t rcvmsg;
    (void)zmqDealer_.recv(rcvmsg);
    const vector<uint8_t> repmsg(reinterpret_cast<uint8_t*>(rcvmsg.data()),
                                    reinterpret_cast<uint8_t*>(rcvmsg.data()) + rcvmsg.size());
    return repmsg;
}

std::optional<_0406_0460_Summary> Poller::doPoll_0406_0460()
{
try
{
    const uint16_t requestRegisterNum{ 0x0460 - 0x0406 + 1 };
    auto rawMessage = miscellaneous::createModbusRtuReadFrame(0x01, 0x03, 0x0406, requestRegisterNum);

    vector<uint8_t> respondMessage;
    auto pollResult = pollMessage(rawMessage);
    if(!pollResult.has_value())
        throw std::logic_error("未接收到有效数据");
    const vector<uint8_t> repmsg = pollResult.value();

    uint16_t rawRegistersNum = (repmsg.size() - 5) / sizeof(uint16_t);
    if(rawRegistersNum != requestRegisterNum)
        throw std::logic_error("数据格式错误");

    vector<uint16_t> rawRegisters(rawRegistersNum);
    ::memcpy(rawRegisters.data(), repmsg.data() + 3, repmsg.size() - 5);
    vector<uint16_t> hostEndianRegisters;
    for(auto itr = rawRegisters.begin(); itr != rawRegisters.end(); itr++){
        const uint16_t hostEndianData = be16toh(*itr);
        hostEndianRegisters.push_back(hostEndianData);
    }
    return createSummary_0406_0460(hostEndianRegisters);
}
catch(const std::exception& e){
    log_.debugStream() << e.what();
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
try
{
    const uint16_t requestRegisterNum{ 0x04D0 - 0x0474 + 1 };
    auto reqmsg = miscellaneous::createModbusRtuReadFrame(0x01, 0x03, 0x0474, requestRegisterNum);
    auto pollResult = pollMessage(reqmsg);
    if(!pollResult.has_value())
        throw std::logic_error("未接收到有效数据");
    const vector<uint8_t> repmsg = pollResult.value();
    uint16_t rawRegistersNum = (repmsg.size() - 5) / sizeof(uint16_t);
    if(rawRegistersNum != requestRegisterNum)
        throw std::logic_error("数据格式错误");

    vector<uint16_t> rawRegisters(rawRegistersNum);
    ::memcpy(rawRegisters.data(), repmsg.data() + 3, repmsg.size() - 5);

    vector<uint16_t> hostEndianRegisters;
    for(auto itr = rawRegisters.begin(); itr != rawRegisters.end(); itr++){
        const uint16_t hostEndianData = be16toh(*itr);
        hostEndianRegisters.push_back(hostEndianData);
    }
    return createSummary_0474_04D0(hostEndianRegisters);
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
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
