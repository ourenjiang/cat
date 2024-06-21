#include "utils/SyncSocketRespond.h"
#include "ems/pcs/Simulator.h"

using namespace std;
using namespace ems;

int main(int argc, char **argv)
{

    // pcs::Simulator 
    auto pcsSimulator = make_unique<pcs::Simulator>();
    SyncSocketRespond syncRespond(argv[1],
                                    std::bind(&pcs::Simulator::getRespondFrame,
                                                                pcsSimulator.get(),
                                                                std::placeholders::_1));
}
