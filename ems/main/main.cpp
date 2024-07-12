#include "utils/ResourceInit.h"
// #include "ems/station/StationBuilder.h"
#include "utils/SyncRespond.h"
#include "ems/pcs/Simulator.h"
#include "ems/bau/Simulator.h"
#include "ems/interface/InterfaceBuilder.h"
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

static shared_ptr<Interface> createInterface(shared_ptr<zmq::socket_t> dealerForInterface)
{
    InterfaceInitParams interfaceParams{
        .httpListenPort = 8902,
    };
    InterfaceBuilder builder2;
    builder2.prebuildHttpServer(interfaceParams.httpListenPort);
    builder2.prebuildDealerForStation(dealerForInterface);
    return builder2.build();
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

    auto stationDealerForBau0 = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    stationDealerForBau0->set(zmq::sockopt::routing_id, "BAU0");
    stationDealerForBau0->connect("tcp://127.0.0.1:9005");

    const string bauProxyAddr = getProxyAddress("BAU");
    auto modubsProxy = make_shared<SyncRequest>(bauProxyAddr);
    modubsProxy->start();
    bau::BauCollector bauCollector(stationDealerForBau0, modubsProxy);
    bauCollector.start();
    //////////////////////////////////////////////////////////////

    auto stationDealerForPcs0 = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    stationDealerForPcs0->set(zmq::sockopt::routing_id, "PCS0");
    stationDealerForPcs0->connect("tcp://127.0.0.1:9005");

    const string pcsProxyAddr = getProxyAddress("PCS");
    auto pcsModubsProxy = make_shared<SyncRequest>(pcsProxyAddr);
    pcsModubsProxy->start();
    pcs::PcsCollector pcsCollector(stationDealerForPcs0, pcsModubsProxy);
    pcsCollector.start();
    //////////////////////////////////////////////////////////////

    auto stationDealerForStrategy0 = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    stationDealerForStrategy0->set(zmq::sockopt::routing_id, "Strategy0");
    stationDealerForStrategy0->connect("tcp://127.0.0.1:9005");

    xftg::StrategyXftg xftg(stationDealerForStrategy0);
    xftg.start();
    //////////////////////////////////////////////////////////////

    Station station;
    station.start();
    //////////////////////////////////////////////////////////////

    auto stationDealerForInterface = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    stationDealerForInterface->set(zmq::sockopt::routing_id, "Interface");
    stationDealerForInterface->connect("tcp://127.0.0.1:9005");

    auto interface = createInterface(stationDealerForInterface);
    interface->start();
    //////////////////////////////////////////////////////////////

    auto stationDealerForProcessor = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    stationDealerForProcessor->set(zmq::sockopt::routing_id, "Processor");
    stationDealerForProcessor->connect("tcp://127.0.0.1:9005");

    auto processor = make_shared<Processor>(stationDealerForProcessor);
    processor->start();
    //////////////////////////////////////////////////////////////

}
