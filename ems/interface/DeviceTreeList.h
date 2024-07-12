#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "ems/station/Model.h"
#include "zmq.hpp"

namespace ems
{
using namespace std;

class DeviceTreeList
{
public:
    DeviceTreeList();
    void requestCallback(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
private:
};

}//namespace ems
