#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "ems/station/Model.h"
#include "zmq.hpp"

namespace ems
{
using namespace std;
using namespace httplib;

class RealtimeWarning
{
public:
    RealtimeWarning();
    void requestCallbackGetWarningDeviceTree(const Request &req, Response &res,
                                            shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
private:
};

}//namespace ems
