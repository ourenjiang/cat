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
    void requestCallback(const httplib::Request &req, httplib::Response &res,
                        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
};
}//namespace interface
