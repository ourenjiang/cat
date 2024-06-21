#pragma once
#include <vector>
#include <thread>
#include <memory>
#include <optional>

#include "boost/asio.hpp"
#include "utils/Log4cppWrapper.h"
#include "utils/ZmqRequest.h"
#include "utils/ZmqPublish.h"
#include <functional>
#include "msgpack.hpp"

/**
 * 使用zeromq，向外发布本模块的状态
 * 通过一个集中式监视器，订阅本模块的发布状态，出现接收超时则判定本模块失效。
*/
namespace ems
{
namespace bau
{
using namespace std;

struct BauStatusSummary
{
    uint16_t bcuBingjiNum;//  簇并机数量
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
    
    double chargeCapacitySum;    // 累计充电电量
    double dischargeCapacitySum; // 累计放电电量

    double pcsRequestChargeCur;// PCS请求充电电流
    double pcsRequestChargeVolt;// PCS请求充电电压
    double pcsRequestDischargeCur;// PCS请求放电电流
    double pcsRequestDischargeVolt;// PCS请求放电电压

    double allowChargeCapacity;   // 可充电量
    double allowDischargeCapacity;// 可放电量

    uint16_t bcuNum;//BCU数量
    uint16_t bcuOnlineNum;//BCU在线数量

    uint16_t bcuSize;//单个BCU管理的BMU数量
    uint16_t bmuCellVoltSize;//单个BMU电芯电压数量
    uint16_t bmuCellTemSize;//单个BMU电芯温度数量
    uint16_t bmuTerminalTemSize;//单个BMU端子温度数量
    vector<int> bcuIndexListOnline;// 在线BCU编号列表

    /******** 以下为统计值 ********/
    size_t warnCountL1;//1级告警数量
    size_t warnCountL2;//2级告警数量
    size_t warnCountL3;//3级告警数量
    size_t warnCountTotal;// 告警总数
    uint16_t cellNum;// 单体数量

    MSGPACK_DEFINE(bcuBingjiNum, protectStatusL1, protectStatusL2, protectStatusL3, faultStatus,
                    volt, cur, soc, soh,
                    cellvoltMax, cellvoltMaxAddr,
                    cellvoltMin, cellvoltMinAddr,
                    celltemMax, celltemMaxAddr,
                    celltemMin, celltemMinAddr,
                    chargeCapacitySum, dischargeCapacitySum,
                    pcsRequestChargeCur, pcsRequestChargeVolt, pcsRequestDischargeCur, pcsRequestDischargeVolt,
                    allowChargeCapacity, allowDischargeCapacity,
                    bcuNum, bcuOnlineNum, bcuSize, bmuCellVoltSize, bmuCellTemSize, bmuTerminalTemSize, bcuIndexListOnline,
                    warnCountL1, warnCountL2, warnCountL3, warnCountTotal, cellNum);
};

struct BingjiStatusSummary
{
    uint32_t protectStatusL1;// 1级保护状态
    uint32_t protectStatusL2;// 2级保护状态
    uint32_t protectStatusL3;// 3级保护状态
    uint32_t faultStatus;    // 故障状态

    size_t warnCountL1;//1级告警数量
    size_t warnCountL2;//2级告警数量
    size_t warnCountL3;//3级告警数量
    MSGPACK_DEFINE(protectStatusL1, protectStatusL2, protectStatusL3, faultStatus,
                   warnCountL1, warnCountL2, warnCountL3);
};

class BauPoller
{
public:
    BauPoller();
    BauPoller(BauPoller&&);
    ~BauPoller();
    void start();
    void setBranchIndex(const int branchIndex){ branchIndex_ = branchIndex; }
    void initPublishInterface(const std::string&, const uint16_t);
    uint16_t getPublishPort(){ return publishPort_; }
private:
    void onTimeout(const boost::system::error_code &error);
    std::optional<vector<uint8_t>> pollMessage(vector<uint8_t>& reqmsg);

    std::optional<BingjiStatusSummary> doFrameBingjiStatus();
    BingjiStatusSummary createBingjiStatusSummary(const vector<uint16_t>&);
    std::optional<BauStatusSummary> doFrameBauStatus();
    BauStatusSummary createBauStatusSummary(const vector<uint16_t>&);
    void publish(const std::string topic, const int location, const BingjiStatusSummary& bingjiStatusSummary, const BauStatusSummary& bauStatusSummary);

    uint16_t getU16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);
    uint32_t getU32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);
    int32_t getS32(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);

    int branchIndex_;
    std::string publishIp_;
    uint16_t publishPort_;
    log4cpp::Category& log_;
    boost::asio::io_service io_service_;
    boost::asio::steady_timer timer_;
    std::shared_ptr<ZmqRequest> ZmqRequest_;
    std::shared_ptr<ZmqPublish> publisher_;
    std::thread loopThread_;
};

}//namespace bau
}//namespace ems
