#include "InterfaceBuilder.h"
#include "boost/assert.hpp"

using namespace std;
using namespace ems;

void InterfaceBuilder::prebuildHttpServer(const int port)
{
    httpServer_ = make_shared<HttpServer>(port);
}

shared_ptr<Interface> InterfaceBuilder::build()
{
    BOOST_ASSERT(dealerForStation_);
    BOOST_ASSERT(httpServer_);

    auto interface = make_shared<Interface>();// 先准备好一个空的Interface
    interface->setDealerForStation(dealerForStation_);
    interface->setHttpServer(httpServer_);
    return interface;
}
