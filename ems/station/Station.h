#pragma once
#include "UserManager.h"
#include "MainWiringDiagram.h"
#include "StorageEnergySystem.h"
#include "PublicInfo.h"
#include "OperationRecord.h"
#include "DeviceTreeList.h"
#include "HttpServer.h"
#include "Poller.h"
#include "HistoryWarning.h"
#include "ems/bau/HeapSystem.h"
#include "ems/bau/CellSystem.h"
#include "ems/bau/RealtimeWarning.h"
#include "ems/bau/Setting.h"

#include "ems/pcs/RealtimeWarning.h"
#include "ems/pcs/Setting.h"

#include "ems/solar/Setting.h"

#include "ems/xftg/DayPlanDuration.h"
#include "ems/xftg/DayPlanProtect.h"
#include "ems/xftg/WeekPlan.h"
#include "ems/xftg/Setting.h"

#include "ems/profit/ProfitHomepage.h"
#include "ems/profit/TypeList.h"
#include "ems/profit/DayPlan.h"
#include "ems/profit/MonthPlan.h"

/**
 * 两件事：
*     1，作为poller消息的订阅端；
 *    2，当策略所依赖的数据完整更新后，主动调用策略模块并获取姿态调整参数，并下发;
 *    3, 启动请求/响应连接的服务端，等待来自外部HTTP请求的转发请求，并返回给HTTP服务器;
*/

namespace ems
{
using namespace std;

class Station
{
public:
    Station();
    ~Station(){
        if(loopThread_.joinable()) loopThread_.join();
    }
    void start();
private:
    void initPollerSubscriberCallbacks();
    void initPollerRespondCallbacks();
    void initBranch(const int branchIndex);
    void initBau(bau::BauInfo& bauInfo, const int branchIndex);
    void initPcs(pcs::PcsInfo& pcsInfo, const int branchIndex);
    void initXftg(xftg::StrategyXftg& xftgInfo, const int branchIndex);
    void initBauStruct(const int, const vector<int>&, const int);
    void initBcuList(map<int, bau::BcuInfo>& bcuList, const vector<int>& bcuIndexListOnline, const int);
    void doXftgStrategy(BranchInfo& branchInfo);
    
    void bauReceiveCallback(const string& sub);
    bool parseBauTopicBauStatus(const string& message);

    void updateBauWarningAndFaultMap(bau::BauInfo& bauInfo, const uint32_t warningLevel1Bits, const uint32_t warningLevel2Bits,
                                        const uint32_t warningLevel3Bits, const uint32_t faultBits);
    
    void bcuReceiveCallback(const string& sub);
    void updateBcuWarningAndFaultMap(bau::BcuInfo& bcuInfo, const uint32_t warningLevel1Bits, const uint32_t warningLevel2Bits,
                                        const uint32_t warningLevel3Bits, const uint32_t faultBits);

    void bmuReceiveCallback(const string& sub);

    void pcsReceiveCallback(const string& sub);
    void updatePcsWarningAndFaultMap(pcs::PcsInfo& pcsInfo, const uint16_t warning1Bits, const uint16_t warning2Bits);

    log4cpp::Category& log_;
    int branchNum_{ 0 };
    std::shared_ptr<StationInfo> stationInfo_;
    Poller poller_;

    bau::HeapSystem heapSystem_;
    bau::CellSystem cellSystem_;
    bau::RealtimeWarning bauRealtimeWarning_;
    bau::Setting bauSetting_;

    pcs::RealtimeWarning pcsRealtimeWarning_;
    pcs::Setting pcsSetting_;

    solar::Setting solarSetting_;

    xftg::DayPlanDuration xftgDayPlanDuration_;
    xftg::DayPlanProtect xftgDayPlanProtect_;
    xftg::WeekPlan xftgWeekPlan_;
    xftg::Setting xftgSetting_;

    UserManager userManager_;
    MainWiringDiagram mainWiringDiagram_;
    StorageEnergySystem storageEnergySystem_;
    Profit profit_;
    PublicInfo publicInfo_;
    HistoryWarning historyWarning_;
    electricity_price::TypeList profitTypeList_;
    electricity_price::DayPlan profitDayPlan_;
    electricity_price::MonthPlan profitMonthPlan_;
    OperationRecord operationRecord_;
    DeviceTreeList deviceTreeList_;

    HttpServer httpServer_;
    std::shared_ptr<ZmqPublish> xftgPublisher_;
    std::thread loopThread_;
};
}//namespace ems
