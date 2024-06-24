#include "BmuPoller.h"
#include <iostream>
#include <sstream>
#include <bitset>
#include "utils/YamlcppWrapper.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "boost/timer/timer.hpp"
#include "utils/Miscellaneous.h"

using namespace std;
using namespace boost;
using namespace ems;
using namespace ems::bau;

BmuPoller::BmuPoller()
    : log_(ems::Log4cppWrapper::getLogger(3))
    , pollerCurrentBcuIndex_(0)
    , pollerCurrentBmuIndex_(0)
    , branchIndex_(0)// 应该从配置文件加载
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

BmuPoller::BmuPoller(BmuPoller&& other)
    : log_(other.log_)
{
}

BmuPoller::~BmuPoller()
{
    if(loopThread_.joinable()) loopThread_.join();
}

void BmuPoller::start()
{
    loopThread_ = thread([this]{
    while(true){
        string rBuffer;
        bool recvResult = bauFrameSubscriber_->recv(rBuffer);
        if(!recvResult){
            /**
             * 接收失败时，不需要对套接字进行重置处理，继续尝试接收.
             * 
            */
            continue;
        }

        this_thread::sleep_for(chrono::milliseconds(200));// 错峰
        const bool result = catchFrameBauBauStatus(rBuffer);
        BOOST_ASSERT(result);

        const uint16_t bcuNum{ bauStatus_.bcuOnlineNum };
        if(bcuNum == 0) return;

        /** 如果实际的单体温度数量为0，就不必再请求数据了 */
        if(bauStatus_.bmuCellTemSize == 0){
            log_.warnStream() << "单体温度有效数量为零, 本次请求取消。";
            continue;
        }

        if(pollerCurrentBmuIndex_ >= bauStatus_.bcuSize){
            pollerCurrentBmuIndex_ = 0;// reset
            if(++pollerCurrentBcuIndex_ >= bauStatus_.bcuOnlineNum){// next
                pollerCurrentBcuIndex_ = 0;// reset
            }
        }

        auto cellvoltSummary = fetchBmuCellvolt(pollerCurrentBcuIndex_,
                 pollerCurrentBmuIndex_,
                 bauStatus_.bmuCellVoltSize);
        auto celltemSummary = fetchBmuCelltem(pollerCurrentBcuIndex_,
                 pollerCurrentBmuIndex_,
                 bauStatus_.bmuCellTemSize, bauStatus_.bmuTerminalTemSize);

        if(!cellvoltSummary.has_value() || !celltemSummary.has_value()){
            continue;
        }

        // 联合发布
        const string topic{ "BmuStatus" };// 主题名称

        string publishContent;
        std::copy(topic.data(), topic.data() + topic.size(), std::back_inserter(publishContent));

        tuple<int, int, int> location{ branchIndex_, pollerCurrentBcuIndex_, pollerCurrentBmuIndex_ };
        auto body = tuple_cat(location, tie(cellvoltSummary.value()), tie(celltemSummary.value()));
        auto serializedBody = msgpackWrapper::pack(body);
        std::copy(serializedBody.data(), serializedBody.data() + serializedBody.size(), std::back_inserter(publishContent));

        if(!bmuCelltemPublisher_->send(publishContent.data(), publishContent.size()))
            log_.errorStream() << topic << " publish failed";
        
        pollerCurrentBmuIndex_++;// 注意递增
    }});
}

void BmuPoller::subscribeBauStatus(const std::string& addr, const std::string& topic)
{
    bauFrameSubscriber_ = make_shared<ZmqSubscribe>(addr);
    bauFrameSubscriber_->subscribe(topic);
}

void BmuPoller::initPublishInterface(const std::string& addr)
{
    // 初始化发布接口
    bmuCelltemPublisher_ = make_shared<ZmqPublish>(addr);
}

bool BmuPoller::catchFrameBauBauStatus(const string& message)
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
    bauStatus_ = bauStatusSummaryNew;
    return true;
}

std::optional<CellvoltSummary> BmuPoller::fetchBmuCellvolt(const uint16_t bcuIndex, const uint16_t bmuIndex, const uint16_t cellvoltNum)
{
    const uint16_t cellvoltBeginAddr{ 0x0700 };
    const uint16_t bcuCapacity{ 20 };
    const uint16_t cellvoltCapacity{ 64 };
    const uint16_t bmuCapacity{ cellvoltCapacity };
    const uint16_t bcuOffset{ static_cast<uint16_t>(bcuIndex * bcuCapacity * bmuCapacity) };
    const uint16_t bmuOffset{ static_cast<uint16_t>(bmuIndex * bmuCapacity) };

    auto rawMessage = miscellaneous::createModbusRtuReadFrame(0x01, 0x03,
                                                                cellvoltBeginAddr + bcuOffset + bmuOffset,
                                                                cellvoltNum);

    // 获取数据
    // vector<uint8_t> respondMessage;
    // if(!pollMessage(rawMessage, respondMessage)) return {};
    // if(respondMessage.empty()){
    //     log_.debug("未接收到有效数据");
    //     return {};
    // }
    auto pollResult = pollMessage(rawMessage);
    if(!pollResult.has_value()) return {};

    auto& msg = pollResult.value();
    BOOST_ASSERT(msg.size() > 5);
    BOOST_ASSERT((msg.size() - 5) % 2 == 0);

    vector<uint16_t> registers;
    for(size_t i = 3; i < msg.size() - 2; i+= 2){
        const uint16_t value = static_cast<uint16_t>(msg[i] << 8 | msg[i+1]);
        registers.push_back(value);
    }
    BOOST_ASSERT(registers.size() == cellvoltNum);
    return createBmuCellvoltSummary(registers);
}

CellvoltSummary BmuPoller::createBmuCellvoltSummary(const vector<uint16_t>& frameRegisters)
{
    CellvoltSummary summary;
    auto& cellvoltList{ summary.cellvoltList };

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

std::optional<CelltemSummary> BmuPoller::fetchBmuCelltem(const uint16_t bcuIndex, const uint16_t bmuIndex, const uint16_t celltemNum, const uint16_t terminaltemNum)
{
    const uint16_t celltemBeginAddr{ 0x6C00 };
    const uint16_t bcuCapacity{ 20 };
    const uint16_t celltemCapacity{ 64 };
    const uint16_t terminaltemCapacity{ 2 };
    const uint16_t bmuCapacity{ celltemCapacity + terminaltemCapacity };
    const uint16_t bcuOffset{ static_cast<uint16_t>(bcuIndex * bcuCapacity * bmuCapacity) };
    const uint16_t bmuOffset{ static_cast<uint16_t>(bmuIndex * bmuCapacity) };

    auto rawMessage = miscellaneous::createModbusRtuReadFrame(0x01, 0x03,
                                                                celltemBeginAddr + bcuOffset + bmuOffset,
                                                                celltemCapacity + terminaltemNum);

    // vector<uint8_t> respondMessage;
    // if(!pollMessage(rawMessage, respondMessage)) return {};
    // if(respondMessage.empty()){
    //     log_.debug("未接收到有效数据");
    //     return {};
    // }
    auto pollResult = pollMessage(rawMessage);
    if(!pollResult.has_value()) return {};

    auto& msg = pollResult.value();
    BOOST_ASSERT(msg.size() > 5);
    BOOST_ASSERT((msg.size() - 5) % 2 == 0);

    vector<uint16_t> registers;
    for(size_t i = 3; i < msg.size() - 2; i+= 2){
        const uint16_t value = static_cast<uint16_t>(msg[i] << 8 | msg[i+1]);
        registers.push_back(value);
    }
    BOOST_ASSERT(registers.size() == celltemCapacity + terminaltemNum);

    // 截取 { 电芯温度数据块, 端子温度数据块 }
    std::vector<uint16_t> celltemData, terminaltemData;
    {
        auto& allTemData = registers;
        move(allTemData.begin(), allTemData.begin() + celltemNum, back_inserter(celltemData));

        const uint16_t celltemCapacity{ 64 };
        std::move(allTemData.begin() + celltemCapacity, allTemData.end(),
                    std::back_inserter(terminaltemData));
    }

    return createBmuCelltemSummary(celltemData, terminaltemData);
}

CelltemSummary BmuPoller::createBmuCelltemSummary(const vector<uint16_t>& cellTemRegisters, const vector<uint16_t>& terminalTemRegisters)
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

std::optional<vector<uint8_t>> BmuPoller::pollMessage(const vector<uint8_t>& reqmsg)
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
