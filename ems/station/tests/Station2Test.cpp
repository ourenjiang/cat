#include "ems/station/Station2.h"
#include "ems/bau/BauCollector2.h"
#include "utils/zeromq.h"
#include "ems/bau/Simulator.h"
#include "utils/SyncRespond.h"

using namespace std;
using namespace ems;

static pair<shared_ptr<bau::Simulator>, shared_ptr<SyncRespond>> createBauSimulator()
{
    // pcs::Simulator 
    auto bauSimulator = make_shared<bau::Simulator>();
    auto bauSyncRespond = make_shared<SyncRespond>("2502",
                    std::bind(&bau::Simulator::getRespondFrame, bauSimulator.get(),
                                                                std::placeholders::_1));
    return { bauSimulator, bauSyncRespond };
}

int main()
{
    auto bauSimulator = createBauSimulator();
    
    auto bauDealerForStation = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    bauDealerForStation->set(zmq::sockopt::routing_id, "BAU0");
    bauDealerForStation->connect("tcp://127.0.0.1:9005");

    const string proxyAddr("127.0.0.1:2502");
    auto modubsProxy = make_shared<SyncRequest>(proxyAddr);
    modubsProxy->start();
    bau::BauCollector2 bauCollector2(bauDealerForStation, modubsProxy);
    bauCollector2.start();

    Station2 station2;
    station2.start();

}
