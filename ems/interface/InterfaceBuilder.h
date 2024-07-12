#pragma once
#include "Interface.h"
// #include "ems/branch/EnergyBranchBuilder.h"

namespace ems
{

struct InterfaceInitParams
{
    int httpListenPort;
};

class InterfaceBuilder
{
public:
    void prebuildDealerForStation(shared_ptr<zmq::socket_t> dealer){ dealerForStation_ = dealer; }
    void prebuildHttpServer(const int port);
    shared_ptr<Interface> build();
private:
    shared_ptr<zmq::socket_t> dealerForStation_;
    shared_ptr<HttpServer> httpServer_;
};

}
