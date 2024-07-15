/**
 * 代理所有的用户请求。
*/
#pragma once
#include "utils/ZmqNode.h"
#include <thread>
#include "HttpServer.h"

#include "UserManager.h"
#include "MainWiringDiagram.h"
#include "StorageEnergySystem.h"
#include "PublicInfo.h"
#include "OperationRecord.h"
#include "DeviceTreeList.h"
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

#include "RealtimeWarning.h"
#include "HistoryWarning.h"

#include "ems/station/Model.h"

namespace ems
{
using namespace std;
using namespace std::placeholders;

class Interface
{
public:
    Interface(shared_ptr<HttpServer> httpServer, shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    ~Interface();
    void start();
private:
    void doRegister();

    shared_ptr<HttpServer> httpServer_;
    shared_ptr<zmq::socket_t> dataDealer_;
    shared_ptr<zmq::socket_t> cmdDealer_;

    // bau::HeapSystem heapSystem_;
    bau::CellSystem cellSystem_;

    bau::RealtimeWarning bauRealtimeWarning_;
    bau::Setting bauSetting_;

    pcs::RealtimeWarning pcsRealtimeWarning_;
    pcs::Setting pcsSetting_;

    solar::Setting solarSetting_;

    UserManager userManager_;
    MainWiringDiagram mainWiringDiagram_;
    StorageEnergySystem storageEnergySystem_;
    Profit profit_;
    PublicInfo publicInfo_;
    HistoryWarning historyWarning_;
    RealtimeWarning realtimeWarning_;

    OperationRecord operationRecord_;
    DeviceTreeList deviceTreeList_;

    std::thread loopThread_;
};

}//namespace ems
