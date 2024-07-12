#pragma once
#include "Station.h"
#include "ems/branch/EnergyBranchBuilder.h"

namespace ems
{

struct StationInitParams
{
    string zmqPort;
    string identityForInterface;
};

class StationBuilder
{
public:
    void prebuildZmqNode(const string& port);
    shared_ptr<zmq::socket_t> getDealer(const string& identity);
    void addBranch(const BranchInitParams& params);
    shared_ptr<Station> build();
private:
    shared_ptr<ZmqNode> zmqNode_; //内部通讯节点
    map<string, shared_ptr<EnergyBranch>> branchList_;
};

}
