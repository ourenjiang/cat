#pragma once
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "zmq.hpp"
#include "ems/station/Model.h"

namespace ems
{
using namespace std;

class PublicInfo
{
public:
    void requestCallback(const httplib::Request &req, httplib::Response &res,
                        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
};
}//namespace ems
