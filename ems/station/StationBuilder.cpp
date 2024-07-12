#include "StationBuilder.h"
#include "boost/assert.hpp"

using namespace std;
using namespace ems;

void StationBuilder::prebuildZmqNode(const string& port)
{
    auto zmqNode = make_shared<ZmqNode>(port);
    zmqNode_ = zmqNode;
}

shared_ptr<zmq::socket_t> StationBuilder::getDealer(const string& identity)
{
    BOOST_ASSERT(zmqNode_);
    return zmqNode_->createDealer(identity);
}

void StationBuilder::addBranch(const BranchInitParams& params)
{
    EnergyBranchBuilder builder;
    builder.prebuildZmqNode(params.zmqPort);
    builder.prebuildBauCollector(params.bauCollectorInitParams);
    builder.prebuildPcsCollector(params.pcsCollectorInitParams);
    builder.prebuildXftg();
    builder.prebuildIndex(params.index);
    builder.prebuildStationDealer(zmqNode_->createDealer("Branch" + to_string(params.index)));
    branchList_[params.identity] = builder.build();
}

shared_ptr<Station> StationBuilder::build()
{
    BOOST_ASSERT(zmqNode_);

    auto station = make_shared<Station>();// 先准备好一个空的Station
    station->setZmqNode(zmqNode_);
    station->setBranchList(branchList_);
    return station;
}
