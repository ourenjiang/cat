#pragma once
#include <vector>
#include <thread>
#include <memory>

#include "utils/Log4cppWrapper.h"
#include "utils/ZmqRequest.h"
#include <functional>
#include "utils/ZmqSubscribe.h"
#include "utils/ZmqPublish.h"
#include "BauPoller.h"

/**
 * 使用zeromq，向外发布本模块的状态
 * 通过一个集中式监视器，订阅本模块的发布状态，出现接收超时则判定本模块失效。
*/
namespace ems
{
namespace bau
{
using namespace std;

using Volt = pair<uint16_t, double>;
using Tem = pair<uint16_t, double>;

struct CellvoltSummary
{
    vector<Volt> cellvoltList;// 电芯电压

    /******** 以下为统计值 ********/
    Volt cellvoltMax;// { 最大单体电压, 最大单体电压下标 }
    Volt cellvoltMin;// { 最小单体电压, 最小单体电压下标 }
    double voltDiff; // 压差

    MSGPACK_DEFINE(cellvoltList,
                   cellvoltMax, cellvoltMin, voltDiff);
};

struct CelltemSummary
{
    vector<Tem> cellTemList;// 电芯温度
    vector<Tem> terminalTemList;// 端子温度

    /******** 以下为统计值 ********/
    Tem celltemMax;// { 最大单体温度, 最大单体温度下标 }
    Tem celltemMin;// { 最小单体温度, 最小单体温度下标 }
    double temDiff; // 温差

    MSGPACK_DEFINE(cellTemList, terminalTemList,
                   celltemMax, celltemMin, temDiff);
};

class BmuPoller
{
public:
    BmuPoller();
    BmuPoller(BmuPoller&&);
    ~BmuPoller();
    void start();
    void subscribeBauStatus(const std::string&, const std::string&);
    void setBranchIndex(const int branchIndex){ branchIndex_ = branchIndex; }
    void initPublishInterface(const std::string&);
private:
    using Mapping = std::unordered_map<std::string, std::string>;

    bool pollMessage(vector<uint8_t>& requestMessage, vector<uint8_t>& respondMessage);
    bool catchFrameBauBauStatus(const std::string&);
    std::optional<CellvoltSummary> fetchBmuCellvolt(const uint16_t bcuIndex, const uint16_t bmuIndex, const uint16_t cellvoltNum);
    CellvoltSummary createBmuCellvoltSummary(const vector<uint16_t>&);

    std::optional<CelltemSummary> fetchBmuCelltem(const uint16_t bcuIndex, const uint16_t bmuIndex, const uint16_t celltemNum, const uint16_t terminaltemNum);
    CelltemSummary createBmuCelltemSummary(const vector<uint16_t>&, const vector<uint16_t>&);

    log4cpp::Category& log_;
    std::shared_ptr<ZmqRequest> ZmqRequest_;
    int pollerCurrentBcuIndex_;
    int pollerCurrentBmuIndex_;
    int branchIndex_;
    std::shared_ptr<ZmqSubscribe> bauFrameSubscriber_;
    std::shared_ptr<ZmqPublish> bmuCelltemPublisher_;
    BauStatusSummary bauStatus_;
    std::thread loopThread_;
};
}//namespace celltem
}//namespace ems
