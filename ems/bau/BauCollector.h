/*
BAU采集器：
    负责创建帧轮询器、计算并发布汇总数据; 
    负责创建从站帧路由器，内部直接透传发送帧；
    负责接收上层接口的控制命令并由命令控制器下发；
*/

#pragma once
#include "Model.h"
#include "zmq.hpp"
#include "utils/SyncRequest.h"
#include <thread>

namespace ems
{
namespace bau
{
using namespace std;
using Volt = pair<uint16_t, double>;
using Tem = pair<uint16_t, double>;

class BauCollector
{
public:
    BauCollector(std::shared_ptr<zmq::socket_t> stationDealer, shared_ptr<SyncRequest> modbusProxy);
    ~BauCollector();

    void start();
private:
    void doWork();
    void doCommand(zmq::message_t& srcIdentity);

    void pollBingjiStatusFrame();
    BingjiStatusSummary createBingjiStatusSummary(const vector<uint16_t>& frameRegisters);

    void pollBauStatusFrame();
    BauStatusSummary createBauStatusSummary(const vector<uint16_t>& frameRegisters);
    BauWarningStatus calcBauUniformWarning(const uint32_t level1Bits, const uint32_t level2Bits, const uint32_t level3Bits);
    BauFaultStatus calcBauFault(const uint32_t faultBits);
    BauRunStatus calcBauRun(const uint32_t faultBits);

    void pollBcuFrame();
    void pollBcuFrame(const int bcuIndex);
    BcuStatusSummary createBcuStatusSummary(const vector<uint16_t>& frameRegisters);
    BcuWarningStatus calcBcuUniformWarning(const uint32_t level1Bits, const uint32_t level2Bits, const uint32_t level3Bits);
    BcuFaultStatus calcBcuFault(const uint32_t faultBits);
    BcuRelayRealStatus calcBcuRelayRealStatus(const uint32_t faultBits);

    void pollBmuFrame();
    void pollBmuFrame(const int bcuIndx, const int bmuIndex);
    void pollBmuCellvolt(const int bcuIndex, const int bmuIndex, const int cellvoltSize);
    void pollBmuCelltem(const int bcuIndex, const int bmuIndex, const int celltemSize, const int terminaltemSize);
    CellvoltSummary createBmuCellvoltSummary(const vector<uint16_t>& frameRegisters);
    CelltemSummary createBmuCelltemSummary(const vector<uint16_t>& cellTemRegisters, const vector<uint16_t>& terminalTemRegisters);

    optional<vector<byte>> sendAndRecv(const vector<uint8_t>& msg);

    void calcWorkAbility();
    bool allowRunning(const uint32_t bauThirdProtectStatus, const uint32_t bauFaultStatus, const uint32_t bingjiThirdProtectStatus, const uint32_t bingjiFaultStatus);
    bool allowCharge(const uint32_t bauThirdProtectStatus, const uint32_t bingjiThirdProtectStatus, const uint32_t specialStatus);// 检查是否有充电能力
    bool allowDischarge(const uint32_t bauThirdProtectStatus, const uint32_t bingjiThirdProtectStatus, const uint32_t specialStatus);// 检查是否有放电能力

    BauInfo bauInfo_;
    std::shared_ptr<zmq::socket_t> stationDealer_;
    shared_ptr<SyncRequest> modbusProxy_;
    vector<int> bcuPollMap_;// BCU状态轮询地址
    vector<int>::iterator bcuPollItr_;
    vector<int> bmuPollMap_;// BMU轮询地址：遍历BCU
    vector<int>::iterator bmuPollItr_;
    vector<int> bmuPollMap2_;// BMU轮询地址：遍历BMU
    vector<int>::iterator bmuPollItr2_;

    std::thread loopThread_;
};

} // namespace bau
} //namespace ems
