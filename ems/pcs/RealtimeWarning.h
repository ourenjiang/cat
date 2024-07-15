#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "zmq.hpp"
#include "ems/station/Model.h"

namespace ems
{
namespace pcs
{
using namespace std;

class RealtimeWarning
{
public:
    RealtimeWarning();
    void requestCallback(const httplib::Request &req, httplib::Response &res,
                        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
private:
};

}//namespace bau
}//namespace ems
