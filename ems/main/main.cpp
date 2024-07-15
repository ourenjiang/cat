#include "utils/ResourceInit.h"
// #include "ems/station/StationBuilder.h"
#include "utils/SyncRespond.h"
#include "ems/pcs/Simulator.h"
#include "ems/bau/Simulator.h"
#include "ems/interface/Interface.h"
#include "ems/processor/Processor.h"
#include "ems/bau/BauCollector.h"
#include "ems/pcs/PcsCollector.h"
#include "ems/station/Station.h"
#include "utils/YamlcppWrapper.h"
#include "ems/xftg/StrategyXftg.h"

using namespace std;
using namespace ems;

static void init()
{
    /**
     * 为了方便测试，这里手动添加环境变量；
     * 实际生产环境部署时应该移除.
    */
    const string envName{ "PACEIC_EMS_SERVER" };
    const string envValue{ "/opt/paceic_ems_server/main" };
    ::setenv(envName.c_str(), envValue.c_str(), 0);

    utils::configurationInitFromEnv();//初始化配置文件
    utils::logInit();// 初始化日志模块
}

static pair<shared_ptr<pcs::Simulator>, shared_ptr<SyncRespond>> createPcsSimulator()
{
    // pcs::Simulator 
    auto pcsSimulator = make_shared<pcs::Simulator>();
    auto pcsSyncRespond = make_shared<SyncRespond>("8001",
                    std::bind(&pcs::Simulator::getRespondFrame, pcsSimulator.get(),
                                                                std::placeholders::_1));
    return { pcsSimulator, pcsSyncRespond };
}

static pair<shared_ptr<bau::Simulator>, shared_ptr<SyncRespond>> createBauSimulator()
{
    // pcs::Simulator 
    auto bauSimulator = make_shared<bau::Simulator>();
    auto bauSyncRespond = make_shared<SyncRespond>("2502",
                    std::bind(&bau::Simulator::getRespondFrame, bauSimulator.get(),
                                                                std::placeholders::_1));
    return { bauSimulator, bauSyncRespond };
}

static void initDatabaseRecord()
{
    xftg::DayPlanDuration::createTable();
    xftg::DayPlanDuration::insertIntoDefaultRecord();
    xftg::DayPlanProtect::createTable();
    xftg::DayPlanProtect::insertIntoDefaultRecord();
    xftg::WeekPlan::createTable();
    xftg::WeekPlan::insertIntoDefaultRecord();
    xftg::Setting::createTable();
    xftg::Setting::insertIntoDefaultRecord();
    /////////////////////////////////////////////////////////////

    electricity_price::DayPlan::createTable();
    electricity_price::DayPlan::insertIntoDefaultRecord();
    electricity_price::MonthPlan::createTable();
    electricity_price::MonthPlan::insertIntoDefaultRecord();
    electricity_price::TypeList::createTable();
    electricity_price::TypeList::insertIntoDefaultRecord();
    /////////////////////////////////////////////////////////////

    HistoryWarning::createTable();
    /////////////////////////////////////////////////////////////

    OperationRecord::createTable();
    OperationRecord::insertIntoDefaultRecord();
    /////////////////////////////////////////////////////////////

    UserManager::createDefaultRecord();
}

static string getProxyAddress(const string& devName)
{
    auto& cfgRoot = YamlcppWrapper::getRoot();
    const auto& collectors = cfgRoot["slave"];
    auto resultLoad = std::find_if(collectors.begin(), collectors.end(), 
        [devName](const YAML::Node& item){
            return item["name"].as<string>() == devName;
    });
    BOOST_ASSERT(resultLoad != collectors.end());
    return (*resultLoad)["address"].as<string>();
}

int main()
{
    init();
    ///////////////////////////////////////////////////

    auto pcsSimulator = createPcsSimulator();
    auto bauSimulator = createBauSimulator();
    ///////////////////////////////////////////////////

    initDatabaseRecord();
    ///////////////////////////////////////////////////

    auto bauDataDealer = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    bauDataDealer->set(zmq::sockopt::routing_id, "BAU0Data");
    bauDataDealer->connect("tcp://127.0.0.1:9005");

    auto bauCmdDealer = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    bauCmdDealer->set(zmq::sockopt::routing_id, "BAU0Cmd");
    bauCmdDealer->connect("tcp://127.0.0.1:9006");

    const string bauProxyAddr = getProxyAddress("BAU");
    auto modubsProxy = make_shared<SyncRequest>(bauProxyAddr);
    modubsProxy->start();
    bau::BauCollector bauCollector(bauDataDealer, bauCmdDealer, modubsProxy);
    bauCollector.start();
    //////////////////////////////////////////////////////////////

    auto pcsDataDealer = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    pcsDataDealer->set(zmq::sockopt::routing_id, "PCS0Data");
    pcsDataDealer->connect("tcp://127.0.0.1:9005");

    auto pcsCmdDealer = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    pcsCmdDealer->set(zmq::sockopt::routing_id, "PCS0Cmd");
    pcsCmdDealer->connect("tcp://127.0.0.1:9006");

    const string pcsProxyAddr = getProxyAddress("PCS");
    auto pcsModubsProxy = make_shared<SyncRequest>(pcsProxyAddr);
    pcsModubsProxy->start();
    pcs::PcsCollector pcsCollector(pcsDataDealer, pcsCmdDealer, pcsModubsProxy);
    pcsCollector.start();
    //////////////////////////////////////////////////////////////

    auto strategyDataDealer = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    strategyDataDealer->set(zmq::sockopt::routing_id, "Strategy0");
    strategyDataDealer->connect("tcp://127.0.0.1:9005");

    auto strategyCmdDealer = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    strategyCmdDealer->set(zmq::sockopt::routing_id, "Strategy0");
    strategyCmdDealer->connect("tcp://127.0.0.1:9006");

    auto strategySetDealer = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    strategySetDealer->set(zmq::sockopt::routing_id, "Strategy0");
    strategySetDealer->connect("tcp://127.0.0.1:9006");

    xftg::StrategyXftg xftg(strategyDataDealer, strategyCmdDealer, strategySetDealer);
    xftg.start();
    //////////////////////////////////////////////////////////////

    Station station;
    station.start();
    //////////////////////////////////////////////////////////////

    auto interfaceDataDealer = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    interfaceDataDealer->set(zmq::sockopt::routing_id, "InterfaceData");
    interfaceDataDealer->connect("tcp://127.0.0.1:9005");

    auto interfaceCmdDealer = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    interfaceCmdDealer->set(zmq::sockopt::routing_id, "InterfaceCmd");
    interfaceCmdDealer->connect("tcp://127.0.0.1:9006");

    auto httpServer = make_shared<HttpServer>(8902);

    auto interface = make_shared<Interface>(httpServer, interfaceDataDealer, interfaceCmdDealer);
    interface->start();
    //////////////////////////////////////////////////////////////

    auto stationDealerForProcessor = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    stationDealerForProcessor->set(zmq::sockopt::routing_id, "Processor");
    stationDealerForProcessor->connect("tcp://127.0.0.1:9005");

    auto processor = make_shared<Processor>(stationDealerForProcessor);
    processor->start();
    //////////////////////////////////////////////////////////////

}
