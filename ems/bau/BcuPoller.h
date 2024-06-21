#pragma once
#include <vector>
#include <thread>
#include <memory>

#include <functional>
#include "utils/Log4cppWrapper.h"
#include "utils/ZmqRequest.h"
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

struct BcuStatusSummary
{
    uint32_t protectStatusL1;// 1级保护状态
    uint32_t protectStatusL2;// 2级保护状态
    uint32_t protectStatusL3;// 3级保护状态
    uint32_t faultStatus;    // 故障状态

    double volt;// 总电压
    double cur;// 总电流
    uint16_t soc;// SOC
    uint16_t soh;// SOH

    double cellvoltMax;    // 最大单体电压
    uint16_t cellvoltMaxAddr;// 最大单体电压地址

    double cellvoltMin;    // 最小单体电压
    uint16_t cellvoltMinAddr;// 最小单体电压地址

    double celltemMax;    // 最大单体温度
    uint16_t celltemMaxAddr;// 最大单体温度地址
    
    double celltemMin;    // 最小单体温度
    uint16_t celltemMinAddr;// 最小单体温度地址
    
    /******** 以下为统计值 ********/
    size_t warnCountL1;//1级告警数量
    size_t warnCountL2;//2级告警数量
    size_t warnCountL3;//3级告警数量
    size_t warnCountTotal;// 告警总数

    bool gridStatus;// 并离网状态

    MSGPACK_DEFINE(protectStatusL1, protectStatusL2, protectStatusL3, faultStatus,
                    volt, cur, soc, soh,
                    cellvoltMax, cellvoltMaxAddr,
                    cellvoltMin, cellvoltMinAddr,
                    celltemMax, celltemMaxAddr,
                    celltemMin, celltemMinAddr,
                    warnCountL1, warnCountL2, warnCountL3, warnCountTotal,
                    gridStatus);
};

class BcuPoller
{
public:
    BcuPoller();
    BcuPoller(BcuPoller&&);
    ~BcuPoller();
    void start();
    void subscribeBauStatus(const std::string&, const std::string&);
    void setBranchIndex(const int branchIndex){ branchIndex_ = branchIndex; }
    void initPublishInterface(const std::string&);
private:
    using Mapping = std::unordered_map<std::string, std::string>;

    std::optional<vector<uint8_t>> pollMessage(vector<uint8_t>& requestMessage);
    bool catchFrameBauBauStatus(const std::string&);
    void doBcu(const uint16_t);
    BcuStatusSummary createBcuStatusSummary(const vector<uint16_t>&);

    uint16_t getU16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);
    uint32_t getU32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);
    int32_t getS32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);

    int branchIndex_;
    std::string publishAddress_;
    log4cpp::Category& log_;
    std::shared_ptr<ZmqRequest> ZmqRequest_;
    // uint16_t pollerCurrentBcuIndex_;
    vector<int> bcuIndexListOnline_;
    vector<int>::iterator bcuIndexListOnlineItr_;
    std::shared_ptr<ZmqSubscribe> bauFrameSubscriber_;
    std::shared_ptr<ZmqPublish> bcuFramePublisher_;
    BauStatusSummary bauStatus_;
    std::thread loopThread_;
};

}//namespace bau
}//namespace ems
