#include "utils/ResourceInit.h"
#include "ems/station/StationBuilder.h"
#include "utils/SyncRespond.h"
#include "ems/pcs/Simulator.h"
#include "ems/bau/Simulator.h"
#include "ems/interface/InterfaceBuilder.h"
#include "ems/processor/Processor.h"

using namespace std;
using namespace ems;

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

int main()
{
    auto pcsSimulator = createPcsSimulator();
    auto bauSimulator = createBauSimulator();
    getchar();
}
