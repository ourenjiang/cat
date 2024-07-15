#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "ems/station/Model.h"
#include "zmq.hpp"

namespace ems
{
namespace bau
{
using namespace std;

class RealtimeWarning
{
public:
    RealtimeWarning();
    void requestCallbackBau(const httplib::Request &req, httplib::Response &res,
                        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    void requestCallbackBcu(const httplib::Request &req, httplib::Response &res,
                        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
private:
};

}//namespace bau
}//namespace ems
