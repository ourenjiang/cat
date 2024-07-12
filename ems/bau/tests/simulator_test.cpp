#include "utils/SyncRespond.h"
#include "ems/bau/Simulator.h"

using namespace std;
using namespace ems;

int main(int argc, char **argv)
{

    // pcs::Simulator 
    auto pcsSimulator = make_unique<bau::Simulator>();
    SyncRespond syncRespond(argv[1],
                                    std::bind(&bau::Simulator::getRespondFrame,
                                                                pcsSimulator.get(),
                                                                std::placeholders::_1));
}
