#pragma once
#include <vector>
#include <thread>
#include <memory>
#include "utils/Log4cppWrapper.h"
#include "utils/ZmqRequest.h"
#include "utils/ZmqPublish.h"
#include "boost/asio.hpp"
#include "msgpack.hpp"

namespace ems
{
namespace pcs
{
using namespace std;

struct _0406_0460_Summary
{
    uint16_t warnStatus1;      // 告警字1
    uint16_t warnStatus2;      // 告警字2
    uint16_t warnStatus3;      // 告警字3
    uint16_t switchStatus;     // 开关状态字
    uint16_t pcsStatus;        // PCS状态字
    uint16_t perihperalStatus; // 外设状态

    double gridLineVoltAB;  // 电网线电压AB
    double gridLineVoltBC;  // 电网线电压BC
    double gridLineVoltCA;  // 电网线电压CA
    double gridCurA;  // 电网电流A
    double gridCurB;  // 电网电流B
    double gridCurC;  // 电网电流C
    double gridTotalActivePower;  // 电网总有功功率
    double gridTotalReactivePower;  // 电网总无功功率
    double gridTotalApparentPower;  // 电网总视在功率

    /******** 以下为统计值 ********/
    size_t warnCountL1;//1级告警数量
    size_t warnCountL2;//2级告警数量
    size_t warnCountL3;//3级告警数量
    size_t warnCountTotal;// 告警总数
    MSGPACK_DEFINE(warnStatus1, warnStatus2, warnStatus3, switchStatus, pcsStatus, perihperalStatus,
                    gridLineVoltAB, gridLineVoltBC, gridLineVoltCA,
                    gridCurA, gridCurB, gridCurC,
                    gridTotalActivePower, gridTotalReactivePower, gridTotalApparentPower);
};

struct _0474_04D0_Summary
{
    double activePowerSetting; // 输出有功功率设置
    uint16_t remoteMode; // 调度方式（0本地，1远程)
    uint16_t powerSetting; // 遥控开关机设置(0x5555 - 变流器开机, 0xAAAA - 变流器关机, 0x55AA - 变流器待机)

    MSGPACK_DEFINE(activePowerSetting, remoteMode, powerSetting);
};

class Poller
{
public:
    Poller();
    Poller(Poller&&);
    ~Poller();
    void start();
    void setBranchIndex(const int branchIndex){ branchIndex_ = branchIndex; }
    void initPublishInterface(const std::string&, const uint16_t);
    uint16_t getPublishPort(){ return publishPort_; }
private:
    void onTimeout(const boost::system::error_code &error);
    void doWork();
    std::optional<vector<uint8_t>> pollMessage(const vector<uint8_t>& reqmsg);

    std::optional<_0406_0460_Summary> doPoll_0406_0460();
    _0406_0460_Summary createSummary_0406_0460(const vector<uint16_t>&);
    
    std::optional<_0474_04D0_Summary> doPoll_0474_04D0();
    _0474_04D0_Summary createSummary_0474_04D0(const vector<uint16_t>&);
    void publish(const string& topic, const int location, const _0406_0460_Summary& obj_0406_0460_Summary, const _0474_04D0_Summary& obj_0474_04D0_Summary);

    int16_t getS16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);
    uint16_t getU16(const uint16_t addr, const uint16_t beginAddr, const vector<uint16_t>& frameRegisters);
    
    int branchIndex_;
    std::string publishIp_;
    uint16_t publishPort_;
    log4cpp::Category& log_;
    boost::asio::io_service io_service_;
    boost::asio::steady_timer timer_;
    // std::unique_ptr<ZmqRequest> requester_;
    zmq::context_t zmqContext_;
    zmq::socket_t zmqDealer_;
    std::unique_ptr<ZmqPublish> publisher_;
    std::thread loopThread_;
};
}//namespace pcs
}//namespace ems