#include "Interface.h"
#include "boost/assert.hpp"
#include <iostream>

using namespace std;
using namespace ems;

Interface::Interface(shared_ptr<HttpServer> httpServer, shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
    : httpServer_(httpServer_)
    , dataDealer_(dataDealer)
    , cmdDealer_(cmdDealer)
{
}

Interface::~Interface()
{
    if(loopThread_.joinable()) loopThread_.join();
}

void Interface::start()
{
    BOOST_ASSERT(httpServer_);

    doRegister();
    httpServer_->start();
}

void Interface::doRegister()
{
    /* 用户管理 */
    httpServer_->setGetCallback("/user/namelist",
        std::bind(&UserManager::getNameListCallback, &userManager_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/user/info/queryAll",
        std::bind(&UserManager::getAllCallback, &userManager_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/user/info/query",
        std::bind(&UserManager::getCallback, &userManager_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/user/info/add",
        std::bind(&UserManager::postCallback, &userManager_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/user/info/modify",
        std::bind(&UserManager::putCallback, &userManager_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/user/info/delete",
        std::bind(&UserManager::deleteCallback, &userManager_, _1, _2, dataDealer_, cmdDealer_));
    
    /* BAU相关 */
    httpServer_->setGetCallback("/heapSystem",
        std::bind(&bau::HeapSystem::requestCallback, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setGetCallback("/cellSystem",
        std::bind(&bau::CellSystem::requestCallback, &cellSystem_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/bau/powerOff",
        std::bind(&bau::Setting::requestCallbackPowerOff, &bauSetting_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/bau/quickStartup",
        std::bind(&bau::Setting::requestCallbackQuickStartup, &bauSetting_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/bau/setBcuRelay",
        std::bind(&bau::Setting::requestCallbackSetBcuRelay, &bauSetting_, _1, _2, dataDealer_, cmdDealer_));
    
    /* PCS相关 */
    httpServer_->setPostCallback("/pcs/setting",
        std::bind(&pcs::Setting::requestCallback, &pcsSetting_, _1, _2, dataDealer_, cmdDealer_));
    
    /* 主页：主接线图 */
    httpServer_->setGetCallback("/mainWiringDiagram",
        std::bind(&MainWiringDiagram::requestCallback, &mainWiringDiagram_, _1, _2, dataDealer_, cmdDealer_));
    /* 主页：收益 */
    httpServer_->setGetCallback("/profit",
        std::bind(&Profit::requestCallback, &profit_, _1, _2, dataDealer_, cmdDealer_));
    /* 主页：储能系统 */
    httpServer_->setGetCallback("/storageEnergySystem",
        std::bind(&StorageEnergySystem::requestCallback, &storageEnergySystem_, _1, _2, dataDealer_, cmdDealer_));
    
    /* 实时告警相关 */
    httpServer_->setGetCallback("/realtimeWarningDeviceTree",
        std::bind(&RealtimeWarning::requestCallbackGetWarningDeviceTree, &realtimeWarning_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setGetCallback("/bauRealtimeWarning",
        std::bind(&bau::RealtimeWarning::requestCallbackBau, &bauRealtimeWarning_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setGetCallback("/bcuRealtimeWarning",
        std::bind(&bau::RealtimeWarning::requestCallbackBcu, &bauRealtimeWarning_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setGetCallback("/pcsRealtimeWarning",
        std::bind(&pcs::RealtimeWarning::requestCallback, &pcsRealtimeWarning_, _1, _2, dataDealer_, cmdDealer_));

    /* 历史告警相关 */
    httpServer_->setPostCallback("/historyWarning",
        std::bind(&HistoryWarning::requestCallbackGet, &historyWarning_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/historyWarningPageInfo",
        std::bind(&HistoryWarning::requestCallbackGetPageInfo, &historyWarning_, _1, _2, dataDealer_, cmdDealer_));

    /* 操作记录相关 */
    httpServer_->setPostCallback("/operationRecord",
        std::bind(&OperationRecord::requestCallbackGet, &operationRecord_, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/operationRecordPageInfo",
        std::bind(&OperationRecord::requestCallbackGetPageInfo, &operationRecord_, _1, _2, dataDealer_, cmdDealer_));

    /* 电价管理-类型列表 */
    using namespace electricity_price;
    httpServer_->setGetCallback("/xftg/electricityPrice/typeList",
        std::bind(&TypeList::requestCallbackGet, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setGetCallback("/xftg/electricityPrice/typeList/namelist",
        std::bind(&TypeList::requestCallbackGetNameList, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/xftg/electricityPrice/typeList",
        std::bind(&TypeList::requestCallbackPost, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPutCallback("/xftg/electricityPrice/typeList",
        std::bind(&TypeList::requestCallbackPut, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setDeleteCallback("/xftg/electricityPrice/typeList",
        std::bind(&TypeList::requestCallbackDelete, _1, _2, dataDealer_, cmdDealer_));
    
    /* 电价管理-月计划 */
    httpServer_->setGetCallback("/xftg/electricityPrice/monthPlan",
        std::bind(&MonthPlan::requestCallbackGet, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/xftg/electricityPrice/monthPlan",
        std::bind(&MonthPlan::requestCallbackPost, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPutCallback("/xftg/electricityPrice/monthPlan",
        std::bind(&MonthPlan::requestCallbackPut, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setDeleteCallback("/xftg/electricityPrice/monthPlan",
        std::bind(&MonthPlan::requestCallbackDelete, _1, _2, dataDealer_, cmdDealer_));
    
    /* 电价管理-日计划 */
    httpServer_->setGetCallback("/xftg/electricityPrice/dayPlan",
        std::bind(&DayPlan::requestCallbackGet, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setGetCallback("/xftg/electricityPrice/dayPlan/namelist",
        std::bind(&DayPlan::requestCallbackGetNameList, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/xftg/electricityPrice/dayPlan",
        std::bind(&DayPlan::requestCallbackPost, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPutCallback("/xftg/electricityPrice/dayPlan",
        std::bind(&DayPlan::requestCallbackPut, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setDeleteCallback("/xftg/electricityPrice/dayPlan",
        std::bind(&DayPlan::requestCallbackDelete, _1, _2, dataDealer_, cmdDealer_));
    
    /* 公共信息 */
    httpServer_->setGetCallback("/branchPublicInfo",
        std::bind(&PublicInfo::requestCallback, &publicInfo_, _1, _2, dataDealer_, cmdDealer_));
    /* 设备状态目录树 */
    httpServer_->setGetCallback("/deviceTreelist",
        std::bind(&DeviceTreeList::requestCallback, &deviceTreeList_, _1, _2, dataDealer_, cmdDealer_));
    
    /* 削峰填谷-日计划时段 */
    httpServer_->setGetCallback("/strategy/xftg/dayPlan",
        std::bind(&xftg::DayPlanDuration::requestCallbackGet, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setGetCallback("/strategy/xftg/dayPlan/namelist",
        std::bind(&xftg::DayPlanDuration::requestCallbackGetNameList, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/strategy/xftg/dayPlan",
        std::bind(&xftg::DayPlanDuration::requestCallbackPost, _1, _2,  dataDealer_, cmdDealer_));
    httpServer_->setPutCallback("/strategy/xftg/dayPlan",
        std::bind(&xftg::DayPlanDuration::requestCallbackPut, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setDeleteCallback("/strategy/xftg/dayPlan",
        std::bind(&xftg::DayPlanDuration::requestCallbackDelete, _1, _2, dataDealer_, cmdDealer_));
    
    /* 削峰填谷-日计划保护参数 */
    httpServer_->setGetCallback("/strategy/protectParams",
        std::bind(&xftg::DayPlanProtect::requestCallbackGet, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setGetCallback("/strategy/protectParams/namelist",
        std::bind(&xftg::DayPlanProtect::requestCallbackGetNameList, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/strategy/protectParams",
        std::bind(&xftg::DayPlanProtect::requestCallbackPost, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPutCallback("/strategy/protectParams",
        std::bind(&xftg::DayPlanProtect::requestCallbackPut, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setDeleteCallback("/strategy/protectParams",
        std::bind(&xftg::DayPlanProtect::requestCallbackDelete, _1, _2, dataDealer_, cmdDealer_));
    
    /* 削峰填谷-周计划 */
    httpServer_->setGetCallback("/strategy/xftg/weekPlan",
        std::bind(&xftg::WeekPlan::requestCallbackGet, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setGetCallback("/strategy/xftg/weekPlan/namelist",
        std::bind(&xftg::WeekPlan::requestCallbackGetNameList, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPostCallback("/strategy/xftg/weekPlan",
        std::bind(&xftg::WeekPlan::requestCallbackPost, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPutCallback("/strategy/xftg/weekPlan",
        std::bind(&xftg::WeekPlan::requestCallbackPut, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setDeleteCallback("/strategy/xftg/weekPlan",
        std::bind(&xftg::WeekPlan::requestCallbackDelete, _1, _2, dataDealer_, cmdDealer_));

    /* 削峰填谷-设置 */
    httpServer_->setGetCallback("/xftg/status",
        std::bind(&xftg::Setting::requestCallbackGet, _1, _2, dataDealer_, cmdDealer_));
    httpServer_->setPutCallback("/xftg/autoRun",
        std::bind(&xftg::Setting::requestCallbackPut, _1, _2, dataDealer_, cmdDealer_));
}
