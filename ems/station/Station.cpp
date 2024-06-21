#include "Station.h"
#include <iostream>
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/xftg/DayPlanDuration.h"
#include "boost/timer/timer.hpp"
#include "utils/Miscellaneous.h"
#include <type_traits>

using namespace ems;

Station::Station()
    : log_(Log4cppWrapper::getLogger(8))
    , stationInfo_(make_shared<StationInfo>())
    , branchNum_(1)
{
    poller_.setStationInfo(stationInfo_);
    initPollerSubscriberCallbacks();
    initPollerRespondCallbacks();

    // 策略消息发布
    xftgPublisher_ = make_shared<ZmqPublish>("tcp://*:9100");

    /** 从配置文件中加载分支数量 */
    // branchNum_ = 1;

    for(int i = 0; i < branchNum_; ++i){
        initBranch(i);
    }
}

void Station::initBranch(const int branchIndex)
{
    auto branchInfo = make_shared<BranchInfo>();
    initBau(branchInfo->bauInfo, branchIndex);
    initPcs(branchInfo->pcsInfo, branchIndex);
    initXftg(branchInfo->xftgStrategy, branchIndex);
    auto& branchList = stationInfo_->branchList;
    branchList.emplace(branchIndex, branchInfo);
}

void Station::initBau(bau::BauInfo& bauInfo, const int branchIndex)
{
    //  start bau pollForward
    auto& pollForward = bauInfo.pollForward;
    pollForward.start();

    //  init bau poller
    auto& bauPoller = bauInfo.bauPoller;
    bauPoller.setBranchIndex(branchIndex);
    bauPoller.initPublishInterface("tcp://localhost:", 6700);
    bauPoller.start();
}

void Station::initPcs(pcs::PcsInfo& pcsInfo, const int branchIndex)
{
    //  start pcs pollForward
    auto& pollForward = pcsInfo.pollForward;
    pollForward.start();

    auto& pcsPoller = pcsInfo.pcsPoller;
    pcsPoller.setBranchIndex(branchIndex);
    pcsPoller.initPublishInterface("tcp://localhost:", 6710);
    pcsPoller.start();
}

void Station::initXftg(xftg::StrategyXftg& xftgInfo, const int branchIndex)
{
    const auto autoRunFlag = xftg::Setting::getRecord(to_string(branchIndex));
    if(autoRunFlag.has_value()){
        auto& status = autoRunFlag.value();
        if(status == "true")
            xftgInfo.setAutoRun(true);
        else if(status == "false")
            xftgInfo.setAutoRun(false);
    }

    // 启动策略服务
    xftgInfo.start();
}

void Station::initBauStruct(const int branchIndex, const int bcuNum, const int bmuNum)
{
    auto& branchList = stationInfo_->branchList;
    auto branchItr = branchList.find(branchIndex);
    if(branchItr == branchList.end()){
        return;// error
    }

    auto& branchInfo = branchItr->second;
    auto& bauInfo = branchInfo->bauInfo;
    if(bauInfo.inited) return;

    auto& bcuList = bauInfo.bcuList;
    BOOST_ASSERT(bcuList.empty());
    for(size_t i = 0; i < bcuNum; ++i){
        bcuList.emplace(i, bau::BcuInfo());

        auto& bcuInfo = bcuList[i];
        auto& bmuList = bcuInfo.bmuList;
        BOOST_ASSERT(bmuList.empty());
        for(size_t j = 0; j < bmuNum; ++j){
            bmuList.emplace(j, bau::BmuInfo());
        }
    }

    auto& bauPoller = bauInfo.bauPoller;
    uint16_t bauPublishPort = bauPoller.getPublishPort();

    // init bcu poller
    auto& bcuPoller = bauInfo.bcuPoller;
    bcuPoller.subscribeBauStatus("tcp://localhost:" + to_string(bauPublishPort), "BauSummary");
    bcuPoller.setBranchIndex(branchIndex);
    bcuPoller.initPublishInterface("tcp://localhost:" + to_string(bauPublishPort + 1));
    bcuPoller.start();

    // init bmu poller
    auto& bmuPoller = bauInfo.bmuPoller;
    bmuPoller.subscribeBauStatus("tcp://localhost:" + to_string(bauPublishPort), "BauSummary");
    bmuPoller.setBranchIndex(branchIndex);
    bmuPoller.initPublishInterface("tcp://localhost:" + to_string(bauPublishPort + 2));
    bmuPoller.start();

    bauInfo.inited = true;// 初始化完成
}

void Station::start()
{
    httpServer_.start();
    loopThread_ = std::thread([this]{
    while(true){
        poller_.doPoll();

        // 基于更新后的数据，执行策略
        for(auto& item : stationInfo_->branchList){
            doXftgStrategy(*item.second);
        }
    }});
}

void Station::doXftgStrategy(BranchInfo& branchInfo)
{
    const auto& bau = branchInfo.bauInfo;
    const auto& pcs = branchInfo.pcsInfo;
    auto& strategy = branchInfo.xftgStrategy;

    // 提取策略所依赖的系统实时参数
    const double batteryCurrentVolt = bau.bauStatusSummary.volt;
    const double batteryCurrentCur = bau.bauStatusSummary.cur;
    const double batteryCurrentPower = batteryCurrentVolt * batteryCurrentCur * 0.001;// kW
    const double batterySuggestChargeVolt = bau.bauStatusSummary.pcsRequestChargeVolt;
    const double batterySuggestChargeCur = bau.bauStatusSummary.pcsRequestChargeCur;
    const double batterySuggestDischargeVolt = bau.bauStatusSummary.pcsRequestDischargeVolt;
    const double batterySuggestDischargeCur = bau.bauStatusSummary.pcsRequestDischargeCur;
    const int batteryCurrentSoc = bau.bauStatusSummary.soc;
    const uint32_t batteryBauThirdProtectStatus = bau.bauStatusSummary.protectStatusL3;
    const uint32_t batteryBauFaultStatus = bau.bauStatusSummary.faultStatus;
    const uint32_t batteryBingjiThirdProtectStatus = bau.bingjiStatusSummary.protectStatusL3;
    const uint32_t batteryBingjiFaultStatus = bau.bingjiStatusSummary.faultStatus;

    const double pcsCurrentSettingPower = pcs.frame_0474_04D0_summary.activePowerSetting;
    const double pcsCurrentOutputPower = pcs.frame_0474_04D0_summary.activePowerSetting;
    string pcsCurrentStatus{ "standby" };
    if(pcsCurrentSettingPower > 0.0)
        pcsCurrentStatus = "discharge";
    else if(pcsCurrentSettingPower < 0.0)
        pcsCurrentStatus = "charge";

    // 测试数据
    pcsCurrentStatus = "discharge";

    // 提取策略所依赖的用户计划参数
    const auto& [durationInfo, protectParams] = strategy.getTarget();
    const string userSuggestStatus = durationInfo.controlType;
    const double userSuggestPower = durationInfo.targetPower;
    const int userSuggestSoc = durationInfo.targetSoc;
    const double allowChargePowerMax = protectParams.chargePowerMax;
    const double allowDischargePowerMax = protectParams.dischargePowerMax;
    const int allowSocMax = protectParams.socMax;
    const int allowSocMin = protectParams.socMin;

    xftg::ExecuteParams params{batteryCurrentVolt, batteryCurrentCur, batteryCurrentPower, batteryCurrentSoc,
                                batterySuggestChargeVolt, batterySuggestChargeCur, batterySuggestDischargeVolt, batterySuggestDischargeCur,
                                pcsCurrentSettingPower, pcsCurrentOutputPower, pcsCurrentStatus,
                                userSuggestStatus, userSuggestPower, userSuggestSoc,
                                allowChargePowerMax, allowDischargePowerMax, allowSocMax, allowSocMin,
                                batteryBauThirdProtectStatus, batteryBauFaultStatus, batteryBingjiThirdProtectStatus, batteryBingjiFaultStatus };
    
    // strategy.doWork(params);
    // 改为发布
    auto serializedBody = msgpackWrapper::pack(params);//ExecStrategy

    const string topic{ "ExecStrategy" };
    vector<byte> msgBuffer(reinterpret_cast<const byte*>(topic.data()),
                                reinterpret_cast<const byte*>(topic.data() + topic.size()));
    std::copy(reinterpret_cast<const byte*>(serializedBody.data()),
                reinterpret_cast<const byte*>(serializedBody.data() + serializedBody.size()), std::back_inserter(msgBuffer));
    xftgPublisher_->send(msgBuffer.data(), msgBuffer.size());
}

void Station::initPollerSubscriberCallbacks()
{
    using namespace std::placeholders;
    poller_.addSubscriber("tcp://127.0.0.1:6700", { "BauSummary" },
            bind(&Station::bauReceiveCallback, this, _1));
    poller_.addSubscriber("tcp://127.0.0.1:6701", { "BcuStatus" },
            bind(&Station::bcuReceiveCallback, this, _1));
    poller_.addSubscriber("tcp://127.0.0.1:6702", { "BmuStatus" },
            bind(&Station::bmuReceiveCallback, this, _1));
    poller_.addSubscriber("tcp://127.0.0.1:6710", { "PcsSummary" },
            bind(&Station::pcsReceiveCallback, this, _1));
}

void Station::initPollerRespondCallbacks()
{
    using namespace std::placeholders;
    poller_.addRespondCallback("CellSystemGet",
            bind(&bau::CellSystem::respondCallback, &cellSystem_, _1, _2));
    poller_.addRespondCallback("HeapSystemGet", 
            bind(&bau::HeapSystem::respondCallback, &heapSystem_, _1, _2));
    poller_.addRespondCallback("BauRealtimeWarning",
            bind(&bau::RealtimeWarning::respondCallbackBau, &bauRealtimeWarning_, _1, _2));
    poller_.addRespondCallback("BcuRealtimeWarning",
            bind(&bau::RealtimeWarning::respondCallbackBcu, &bauRealtimeWarning_, _1, _2));
    poller_.addRespondCallback("BauSettingPowerOff",
            bind(&bau::Setting::respondCallbacPowerOff, &bauSetting_, _1, _2));
    poller_.addRespondCallback("BauSettingQuickStartup",
            bind(&bau::Setting::respondCallbacQuickStartup, &bauSetting_, _1, _2));
    poller_.addRespondCallback("BauSettingSetBcuRelay",
            bind(&bau::Setting::respondCallbacSetBcuRelay, &bauSetting_, _1, _2));
    
    poller_.addRespondCallback("PcsRealtimeWarning", 
            bind(&pcs::RealtimeWarning::respondCallback, &pcsRealtimeWarning_, _1, _2));
    poller_.addRespondCallback("PcsSetting",
            bind(&pcs::Setting::respondCallback, &pcsSetting_, _1, _2));
    
    poller_.addRespondCallback("MainWiringDiagramGet", 
            bind(&MainWiringDiagram::respondCallback, &mainWiringDiagram_, _1, _2));
    poller_.addRespondCallback("StorageEnergySystemGet", 
            bind(&StorageEnergySystem::respondCallback, &storageEnergySystem_, _1, _2));
    poller_.addRespondCallback("ProfitHomepageGet", 
            bind(&Profit::respondCallback, &profit_, _1, _2));
    poller_.addRespondCallback("BranchPublicInfoGet", 
            bind(&PublicInfo::respondCallback, &publicInfo_, _1, _2));
    
    /* 操作记录 */
    poller_.addRespondCallback("OperationRecordGet", 
            bind(&OperationRecord::respondCallbackGet, &operationRecord_, _1, _2));
    poller_.addRespondCallback("OperationRecordPageInfoGet",
            bind(&OperationRecord::respondCallbackGetPageInfo, &operationRecord_, _1, _2));
    
    // 历史告警记录
    poller_.addRespondCallback("WarningDeviceTree", 
            bind(&HistoryWarning::respondCallbackGetWarningDeviceTree, &historyWarning_, _1, _2));
    poller_.addRespondCallback("WarningRecordGet", 
            bind(&HistoryWarning::respondCallbackGet, &historyWarning_, _1, _2));
    poller_.addRespondCallback("WarningRecordPageInfoGet", 
            bind(&HistoryWarning::respondCallbackGetPageInfo, &historyWarning_, _1, _2));
    
    /* 削峰填谷日计划 */
    poller_.addRespondCallback("XftgDayPlanDurationDelete", 
            bind(&xftg::DayPlanDuration::respondCallbackDelete, &xftgDayPlanDuration_, _1, _2));
    poller_.addRespondCallback("XftgDayPlanDurationPut",
            bind(&xftg::DayPlanDuration::respondCallbackPut, &xftgDayPlanDuration_, _1, _2));
    poller_.addRespondCallback("XftgDayPlanDurationPost",
            bind(&xftg::DayPlanDuration::respondCallbackPost, &xftgDayPlanDuration_, _1, _2));
    
    /* 削峰填谷保护计划 */
    poller_.addRespondCallback("XftgDayPlanProtectDelete", 
            bind(&xftg::DayPlanProtect::respondCallbackDelete, &xftgDayPlanProtect_, _1, _2));
    poller_.addRespondCallback("XftgDayPlanProtectPost", 
            bind(&xftg::DayPlanProtect::respondCallbackPost, &xftgDayPlanProtect_, _1, _2));
    poller_.addRespondCallback("XftgDayPlanProtectPut", 
            bind(&xftg::DayPlanProtect::respondCallbackPut, &xftgDayPlanProtect_, _1, _2));
    
    /* 削峰填谷周计划 */
    poller_.addRespondCallback("XftgWeekPlanDelete", 
            bind(&xftg::WeekPlan::respondCallbackDelete, &xftgWeekPlan_, _1, _2));
    poller_.addRespondCallback("XftgWeekPlanPost", 
            bind(&xftg::WeekPlan::respondCallbackPost, &xftgWeekPlan_, _1, _2));
    poller_.addRespondCallback("XftgWeekPlanPut", 
            bind(&xftg::WeekPlan::respondCallbackPut, &xftgWeekPlan_, _1, _2));
    
    /* 削峰填谷自动运行标志 */
    poller_.addRespondCallback("XftgAutoRunPut",
            bind(&xftg::Setting::respondPutCallback, &xftgSetting_, _1, _2));
    
    /* 电价日计划 */
    poller_.addRespondCallback("ElectricityPriceDayPlanPost", 
            bind(&electricity_price::DayPlan::respondCallbackPost, &profitDayPlan_, _1, _2));
    poller_.addRespondCallback("ElectricityPriceDayPlanDelete", 
            bind(&electricity_price::DayPlan::respondCallbackDelete, &profitDayPlan_, _1, _2));
    poller_.addRespondCallback("ElectricityPriceDayPlanPut", 
            bind(&electricity_price::DayPlan::respondCallbackPut, &profitDayPlan_, _1, _2));
    
    /* 电价月计划 */
    poller_.addRespondCallback("ElectricityPriceMonthPlanPost", 
            bind(&electricity_price::MonthPlan::respondCallbackPost, &profitMonthPlan_, _1, _2));
    poller_.addRespondCallback("ElectricityPriceMonthPlanDelete", 
            bind(&electricity_price::MonthPlan::respondCallbackDelete, &profitMonthPlan_, _1, _2));
    poller_.addRespondCallback("ElectricityPriceMonthPlanPut", 
            bind(&electricity_price::MonthPlan::respondCallbackPut, &profitMonthPlan_, _1, _2));

    /* 电价类型 */
    poller_.addRespondCallback("ElectricityPriceTypeListPost", 
            bind(&electricity_price::TypeList::respondCallbackPost, &profitTypeList_, _1, _2));
    poller_.addRespondCallback("ElectricityPriceTypeListDelete", 
            bind(&electricity_price::TypeList::respondCallbackDelete, &profitTypeList_, _1, _2));
    poller_.addRespondCallback("ElectricityPriceTypeListPut", 
            bind(&electricity_price::TypeList::respondCallbackPut, &profitTypeList_, _1, _2));
    
    poller_.addRespondCallback("DeviceTreeListGet",
            bind(&DeviceTreeList::respondCallback, &deviceTreeList_, _1, _2));

}

bool Station::parseBauTopicBauStatus(const string& message)
{
    const string topic{ "BauSummary" };
    if(message.compare(0, topic.size(), topic) != 0){
        return false;
    }

    tuple<int, bau::BingjiStatusSummary, bau::BauStatusSummary> requestBody;
    const bool unpackResult = msgpackWrapper::unpack(message.data() + topic.size(),
                                                message.size() - topic.size(), 
                                                requestBody);

    auto& [branchIndex, bingjiStatusSummaryNew, bauStatusSummaryNew] = requestBody;

    auto& branchList = stationInfo_->branchList;
    BOOST_ASSERT(branchList.find(branchIndex) != branchList.end());

    auto& branchInfo = branchList[branchIndex];
    auto& bauInfo = branchInfo->bauInfo;
    auto& bauStatusSummaryCache = bauInfo.bauStatusSummary;
    auto& bingjiStatusSummaryCache = bauInfo.bingjiStatusSummary;
    
    historyWarning_.handleBauStatusSummary(branchIndex, bauStatusSummaryCache, bauStatusSummaryNew);// 生成告警变位记录
    bauStatusSummaryCache = bauStatusSummaryNew;// 更新缓存
    bingjiStatusSummaryCache = bingjiStatusSummaryNew;// 更新缓存
    bauInfo.onlineFlag = true;// 更新设备在线状态
    // 更新三级告警状态
    updateBauWarningAndFaultMap(bauInfo, bauStatusSummaryCache.warnCountL1, bauStatusSummaryCache.warnCountL2,
                                bauStatusSummaryCache.warnCountL3, bauStatusSummaryCache.faultStatus);
    initBauStruct(branchIndex, bauStatusSummaryCache.bcuNum, bauStatusSummaryCache.bcuSize);// 刷新BAU结构

    {
        /** 更新SOC实时曲线 */
        auto& snapshot = stationInfo_->socRealtimeSnapshot;
        if(snapshot.size() >= 10){
            snapshot.pop_front();
        }
        snapshot.emplace_back(miscellaneous::getCurrentTimestamp(), bauStatusSummaryCache.soc);
    }
    {
        /** 更新电流实时曲线 */
        auto& snapshot = stationInfo_->curRealtimeSnapshot;
        if(snapshot.size() >= 10){
            snapshot.pop_front();
        }
        snapshot.emplace_back(miscellaneous::getCurrentTimestamp(), bauStatusSummaryCache.cur);
    }
    {
        /** 更新功率实时曲线 */
        auto& snapshot = stationInfo_->powerRealtimeSnapshot;
        if(snapshot.size() >= 10){
            snapshot.pop_front();
        }
        snapshot.emplace_back(miscellaneous::getCurrentTimestamp(), 
                                bauStatusSummaryCache.volt * bauStatusSummaryCache.cur * 0.001);
    }
    return true;
}

void Station::bauReceiveCallback(const string& message)
{
    if(parseBauTopicBauStatus(message)){
        return;
    }

    {
        const string topic{ "Timeout" };
        if(message.compare(0, topic.size(), topic) != 0){
            return;
        }

        int branchIndex;
        std::memcpy(&branchIndex, message.data() + topic.size(), sizeof(int));
        
        // 更新设备在线状态
        auto& branchList = stationInfo_->branchList;
        auto& branchInfo = branchList[branchIndex];
        auto& bauInfo = branchInfo->bauInfo;
        bauInfo.onlineFlag = false;
    }
}

int getWarningActiveIndex(const int bitIndex, const uint32_t warningLevel1Bits, const uint32_t warningLevel2Bits, const uint32_t warningLevel3Bits)
{
    const bitset<32> warningLevel1BitMap{ warningLevel1Bits };
    const bitset<32> warningLevel2BitMap{ warningLevel2Bits };
    const bitset<32> warningLevel3BitMap{ warningLevel3Bits };

    if(warningLevel3BitMap[bitIndex]){
        return 3;
    }
    if(warningLevel2BitMap[bitIndex]){
        return 2;
    }
    if(warningLevel1BitMap[bitIndex]){
        return 1;
    }
    return 0;
}

void Station::updateBauWarningAndFaultMap(bau::BauInfo& bauInfo, const uint32_t warningLevel1Bits, const uint32_t warningLevel2Bits,
                                        const uint32_t warningLevel3Bits, const uint32_t faultBits)
{
    // 告警
    bau::BauWarningStatus& warningStatus = bauInfo.warningStatus;
    warningStatus.cellVoltHigh = getWarningActiveIndex(0, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.cellVoltLow = getWarningActiveIndex(1, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.totalVoltHigh = getWarningActiveIndex(2, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.totalVoltLow = getWarningActiveIndex(3, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeOverCur = getWarningActiveIndex(4, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeOverCur = getWarningActiveIndex(5, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeTemHigh = getWarningActiveIndex(6, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeTemHigh = getWarningActiveIndex(7, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeTemLow = getWarningActiveIndex(8, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeTemLow = getWarningActiveIndex(9, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.envTemHigh = getWarningActiveIndex(10, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.envTemLow = getWarningActiveIndex(11, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeRelayTemHigh = getWarningActiveIndex(12, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeRelayTemHigh = getWarningActiveIndex(13, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.negativeRelayTemHigh = getWarningActiveIndex(14, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.socHigh = getWarningActiveIndex(15, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.socLow = getWarningActiveIndex(16, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.positiveInsulationLeakage = getWarningActiveIndex(17, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.negativeInsulationLeakage = getWarningActiveIndex(18, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeVoltDiff = getWarningActiveIndex(19, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeVoltDiff = getWarningActiveIndex(20, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeTemDiff = getWarningActiveIndex(21, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeTemDiff = getWarningActiveIndex(22, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.terminalTemHigh = getWarningActiveIndex(23, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.bcuVoltDiff = getWarningActiveIndex(24, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);

    const bitset<32> warningLevel1BitMap{ warningLevel1Bits };
    bauInfo.warningL1Count = warningLevel1BitMap.count();
    const bitset<32> warningLevel2BitMap{ warningLevel2Bits };
    bauInfo.warningL2Count = warningLevel2BitMap.count();
    const bitset<32> warningLevel3BitMap{ warningLevel3Bits };
    bauInfo.warningL3Count = warningLevel3BitMap.count();

    // 故障
    bau::BauFaultStatus& faultStatus = bauInfo.faultStatus;
    const bitset<32> faultStatusBitMap{ faultBits };
    faultStatus.canBusErr = faultStatusBitMap[0];
    faultStatus.rs485Err = faultStatusBitMap[1];
    faultStatus.bcuVersionErr = faultStatusBitMap[2];

    const bitset<32> faultBitMap{ faultBits };
    bauInfo.faultCount = faultBitMap.count();
}

void Station::updateBcuWarningAndFaultMap(bau::BcuInfo& bcuInfo, const uint32_t warningLevel1Bits, const uint32_t warningLevel2Bits,
                                        const uint32_t warningLevel3Bits, const uint32_t faultBits)
{
    // 告警
    bau::BcuWarningStatus& warningStatus = bcuInfo.warningStatus;
    warningStatus.cellVoltHigh = getWarningActiveIndex(0, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.cellVoltLow = getWarningActiveIndex(1, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.totalVoltHigh = getWarningActiveIndex(2, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.totalVoltLow = getWarningActiveIndex(3, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeOverCur = getWarningActiveIndex(4, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeOverCur = getWarningActiveIndex(5, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeTemHigh = getWarningActiveIndex(6, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeTemHigh = getWarningActiveIndex(7, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeTemLow = getWarningActiveIndex(8, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeTemLow = getWarningActiveIndex(9, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.envTemHigh = getWarningActiveIndex(10, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.envTemLow = getWarningActiveIndex(11, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeRelayTemHigh = getWarningActiveIndex(12, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeRelayTemHigh = getWarningActiveIndex(13, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.negativeRelayTemHigh = getWarningActiveIndex(14, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.socHigh = getWarningActiveIndex(15, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.socLow = getWarningActiveIndex(16, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.positiveInsulationLeakage = getWarningActiveIndex(17, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.negativeInsulationLeakage = getWarningActiveIndex(18, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeVoltDiff = getWarningActiveIndex(19, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeVoltDiff = getWarningActiveIndex(20, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.chargeTemDiff = getWarningActiveIndex(21, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.dischargeTemDiff = getWarningActiveIndex(22, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);

    warningStatus.cellTemIncrease = getWarningActiveIndex(23, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.cellSampling = getWarningActiveIndex(24, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.ntcSamplingErr = getWarningActiveIndex(25, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);
    warningStatus.terminalTemHigh = getWarningActiveIndex(26, warningLevel1Bits, warningLevel2Bits, warningLevel3Bits);

    const bitset<32> warningLevel1BitMap{ warningLevel1Bits };
    bcuInfo.warningL1Count = warningLevel1BitMap.count();
    const bitset<32> warningLevel2BitMap{ warningLevel2Bits };
    bcuInfo.warningL2Count = warningLevel2BitMap.count();
    const bitset<32> warningLevel3BitMap{ warningLevel3Bits };
    bcuInfo.warningL3Count = warningLevel3BitMap.count();

    // 故障
    bau::BcuFaultStatus& faultStatus = bcuInfo.faultStatus;
    const bitset<32> faultStatusBitMap{ faultBits };
    faultStatus.chargeRelayCombine = faultStatusBitMap[0];
    faultStatus.chargeRelayDisabled = faultStatusBitMap[1];
    faultStatus.dischargeRelayCombine = faultStatusBitMap[2];
    faultStatus.dischargeRelayDisabled = faultStatusBitMap[3];
    faultStatus.prechargeRelayCombine = faultStatusBitMap[4];
    faultStatus.prechargeRelayDisabled = faultStatusBitMap[5];
    faultStatus.negativeRelayCombine = faultStatusBitMap[6];
    faultStatus.negativeRelayDisabled = faultStatusBitMap[7];
    faultStatus.heatingFilmRelayCombine = faultStatusBitMap[8];
    faultStatus.heatingFileRelayDisabled = faultStatusBitMap[9];
    faultStatus._12VErr = faultStatusBitMap[10];
    faultStatus.cellFault = faultStatusBitMap[11];
    faultStatus.prechargeFault = faultStatusBitMap[12];
    faultStatus.heatingFilmFault = faultStatusBitMap[13];
    faultStatus.insulationBoardCommFault = faultStatusBitMap[14];
    faultStatus.samplingBoardCommFault = faultStatusBitMap[15];
    faultStatus.curDiverterFault = faultStatusBitMap[16];
    faultStatus.ntcFault = faultStatusBitMap[17];

    const bitset<32> faultBitMap{ faultBits };
    bcuInfo.faultCount = faultBitMap.count();
}


void Station::updatePcsWarningAndFaultMap(pcs::PcsInfo& pcsInfo, const uint16_t warning1Bits, const uint16_t warning2Bits)
{
    const bitset<32> warning1BitMap{ warning1Bits };
    const bitset<32> warning2BitMap{ warning2Bits };

    // 告警
    pcs::WarningStatus& warningStatus = pcsInfo.warningStatus;
    warningStatus.warning1_invertOverCur = warning1BitMap[0];
    warningStatus.warning1_batteryVoltLow = warning1BitMap[3];
    warningStatus.warning1_batteryChargeDisabled = warning1BitMap[4];
    warningStatus.warning1_dcGeneratrixOverVolt = warning1BitMap[6];
    warningStatus.warning1_dcGeneratrixShortCircuit = warning1BitMap[7];
    warningStatus.warning1_outputContactorOpenCircuit = warning1BitMap[8];
    warningStatus.warning1_outputContactorShortCircuit = warning1BitMap[9];
    warningStatus.warning1_converterOverTem = warning1BitMap[10];
    warningStatus.warning1_outputOverLoad = warning1BitMap[11];

    warningStatus.warning2_gridOverVolt = warning2BitMap[0];
    warningStatus.warning2_gridLackVolt = warning2BitMap[1];
    warningStatus.warning2_gridPhaseOrderReverse = warning2BitMap[2];
    warningStatus.warning2_gridIslandingEffectProtect = warning2BitMap[4];
    warningStatus.warning2_batteryDischargeDisabled = warning2BitMap[8];
    warningStatus.warning2_energentPowerOff = warning2BitMap[13];
    warningStatus.warning2_converterNotSync = warning2BitMap[14];

    /** 这里按等级进行告警数量的累加统计时，将所有告警视为3级告警类型 */
    pcsInfo.warningL1Count = 0;
    pcsInfo.warningL2Count = 0;
    pcsInfo.warningL3Count = 0;
    if(warningStatus.warning1_invertOverCur) ++pcsInfo.warningL3Count;
    if(warningStatus.warning1_batteryVoltLow) ++pcsInfo.warningL3Count;
    if(warningStatus.warning1_batteryChargeDisabled) ++pcsInfo.warningL3Count;
    if(warningStatus.warning1_dcGeneratrixOverVolt) ++pcsInfo.warningL3Count;
    if(warningStatus.warning1_dcGeneratrixShortCircuit) ++pcsInfo.warningL3Count;
    if(warningStatus.warning1_outputContactorOpenCircuit) ++pcsInfo.warningL3Count;
    if(warningStatus.warning1_outputContactorShortCircuit) ++pcsInfo.warningL3Count;
    if(warningStatus.warning1_converterOverTem) ++pcsInfo.warningL3Count;
    if(warningStatus.warning1_outputOverLoad) ++pcsInfo.warningL3Count;
    if(warningStatus.warning2_gridOverVolt) ++pcsInfo.warningL3Count;
    if(warningStatus.warning2_gridLackVolt) ++pcsInfo.warningL3Count;
    if(warningStatus.warning2_gridPhaseOrderReverse) ++pcsInfo.warningL3Count;
    if(warningStatus.warning2_gridIslandingEffectProtect) ++pcsInfo.warningL3Count;
    if(warningStatus.warning2_batteryDischargeDisabled) ++pcsInfo.warningL3Count;
    if(warningStatus.warning2_energentPowerOff) ++pcsInfo.warningL3Count;
    if(warningStatus.warning2_converterNotSync) ++pcsInfo.warningL3Count;

    // 故障
    pcs::FaultStatus& faultStatus = pcsInfo.faultStatus;
    faultStatus.warning1_zbLimitCurFault = warning1BitMap[1];
    faultStatus.warning1_converterFault = warning1BitMap[2];
    faultStatus.warning1_bingjiCommFault = warning1BitMap[5];
    faultStatus.warning1_batteryConnectReverse = warning1BitMap[12];
    faultStatus.warning1_dcContactorFault = warning1BitMap[13];
    faultStatus.warning1_bmsCommFault = warning1BitMap[14];
    faultStatus.warning1_inverterLackPhaseFault = warning1BitMap[15];

    faultStatus.warning2_gridFrequencyErr = warning2BitMap[3];
    faultStatus.warning2_drivingLineFault = warning2BitMap[5];
    faultStatus.warning2_lightningProtectFault = warning2BitMap[6];
    faultStatus.warning2_insulationImpedanceErr = warning2BitMap[7];
    faultStatus.warning2_invertOverVoltFault = warning2BitMap[9];
    faultStatus.warning2_15VPowerFault = warning2BitMap[10];
    faultStatus.warning2_acFanFault = warning2BitMap[11];
    faultStatus.warning2_batteryFault = warning2BitMap[12];
    faultStatus.warning2_ctOrHallOpenCircuitFault = warning2BitMap[15];

    /* 故障数量累加统计 */
    pcsInfo.faultCount = 0;
    if(faultStatus.warning1_zbLimitCurFault) ++pcsInfo.faultCount;
    if(faultStatus.warning1_converterFault) ++pcsInfo.faultCount;
    if(faultStatus.warning1_bingjiCommFault) ++pcsInfo.faultCount;
    if(faultStatus.warning1_batteryConnectReverse) ++pcsInfo.faultCount;
    if(faultStatus.warning1_dcContactorFault) ++pcsInfo.faultCount;
    if(faultStatus.warning1_bmsCommFault) ++pcsInfo.faultCount;
    if(faultStatus.warning1_inverterLackPhaseFault) ++pcsInfo.faultCount;
    if(faultStatus.warning2_gridFrequencyErr) ++pcsInfo.faultCount;
    if(faultStatus.warning2_drivingLineFault) ++pcsInfo.faultCount;
    if(faultStatus.warning2_lightningProtectFault) ++pcsInfo.faultCount;
    if(faultStatus.warning2_insulationImpedanceErr) ++pcsInfo.faultCount;
    if(faultStatus.warning2_invertOverVoltFault) ++pcsInfo.faultCount;
    if(faultStatus.warning2_15VPowerFault) ++pcsInfo.faultCount;
    if(faultStatus.warning2_acFanFault) ++pcsInfo.faultCount;
    if(faultStatus.warning2_batteryFault) ++pcsInfo.faultCount;
    if(faultStatus.warning2_ctOrHallOpenCircuitFault) ++pcsInfo.faultCount;
}

void Station::bcuReceiveCallback(const string& message)
{
try
{
    const string topic{ "BcuStatus" };
    BOOST_ASSERT(message.compare(0, topic.size(), topic) == 0);

    tuple<int, int, bau::BcuStatusSummary> requestBody;
    const bool unpackResult = msgpackWrapper::unpack(message.data() + topic.size(),
                                                message.size() - topic.size(), 
                                                requestBody);

    auto& [branchIndex, bcuIndex, summary] = requestBody;

    // 保存
    auto& branchList = stationInfo_->branchList;
    if(branchList.find(branchIndex) == branchList.end()){
        return;
    }
    auto& branchInfo = branchList[branchIndex];
    auto& bauInfo = branchInfo->bauInfo;
    auto& bcuList = bauInfo.bcuList;
    if(bcuList.find(bcuIndex) == bcuList.end()){
        return;
    }
    auto& bcuInfo = bcuList[bcuIndex];
    bcuInfo.base = summary;
    // 更新告警状态
    updateBcuWarningAndFaultMap(bcuInfo,
                                summary.warnCountL1, summary.warnCountL2, summary.warnCountL3, summary.faultStatus);
    // 更新设备在线状态
    bcuInfo.onlineFlag = true;
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void Station::bmuReceiveCallback(const string& message)
{
try
{
    const string topic{ "BmuStatus" };
    BOOST_ASSERT(message.compare(0, topic.size(), topic) == 0);

    tuple<int, int, int, bau::CellvoltSummary, bau::CelltemSummary> body;
    const bool unserializedResult = msgpackWrapper::unpack(message.data() + topic.size(),
                                                            message.size() - topic.size(), body);
    BOOST_ASSERT(unserializedResult);

    auto [branchIndex, bcuIndex, bmuIndex, cellvoltSummary, celltemSummary] = body;
    
    auto& branchList = stationInfo_->branchList;
    if(branchList.find(branchIndex) == branchList.end()){
        throw std::invalid_argument("branchIndex not exist");
    }
    auto& branchInfo = branchList[branchIndex];
    auto& bauInfo = branchInfo->bauInfo;
    auto& bcuList = bauInfo.bcuList;
    if(bcuList.find(bcuIndex) == bcuList.end()){
        throw std::invalid_argument("bcuIndex not exist");
    }
    auto& bcuInfo = bcuList[bcuIndex];
    auto& bmuList = bcuInfo.bmuList;
    if(bmuList.find(bmuIndex) == bmuList.end()){
        throw std::invalid_argument("bmuIndex not exist");
    }
    auto& bmuInfo = bmuList[bmuIndex];
    auto& cellvoltInfo = bmuInfo.cellvoltInfo;
    bmuInfo.cellvoltInfo = cellvoltSummary;
    bmuInfo.celltemInfo = celltemSummary;
}
catch(const std::exception& e){
    log_.errorStream() << e.what();
}
}

void Station::pcsReceiveCallback(const string& message)
{   
    const string topic{ "PcsSummary" };
    if(message.compare(0, topic.size(), topic) != 0){
        return;
    }

    tuple<int, pcs::_0406_0460_Summary, pcs::_0474_04D0_Summary> requestBody;
    const bool unpackResult = msgpackWrapper::unpack(message.data() + topic.size(),
                                                message.size() - topic.size(), 
                                                requestBody);

    auto& [branchIndex, obj_0406_0460_Summary, obj_0474_04D0_Summary] = requestBody;

    auto& branchList = stationInfo_->branchList;
    if(branchList.find(branchIndex) == branchList.end()){
        return;
    }
    auto& branchInfo = branchList[branchIndex];
    auto& pcsInfo = branchInfo->pcsInfo;
    auto& frame_0406_0460_summary = pcsInfo.frame_0406_0460_summary;
    auto& frame_0474_04D0_summary = pcsInfo.frame_0474_04D0_summary;
    frame_0406_0460_summary = obj_0406_0460_Summary;
    frame_0474_04D0_summary = obj_0474_04D0_Summary;
    // 更新告警状态
    updatePcsWarningAndFaultMap(pcsInfo, obj_0406_0460_Summary.warnStatus1, obj_0406_0460_Summary.warnCountL2);
    // 更新设备在线状态
    pcsInfo.onlineFlag = true;
    return;
}
