#pragma once
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "ems/station/Model.h"
#include "zmq.hpp"

namespace ems
{
using namespace std;

class Profit
{
public:
    Profit();
    void requestCallback(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
private:
    log4cpp::Category& log_;
};
}//namespace interface
