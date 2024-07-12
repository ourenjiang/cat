#pragma once
#include "utils/HttpWrapper.h"
#include "zmq.hpp"
#include "ems/station/Model.h"

namespace ems
{
namespace pcs
{
using namespace std;
using namespace httplib;

class Setting
{
public:
    void requestCallback(const Request &req, Response &res, shared_ptr<zmq::socket_t> stationDealer);
private:
    zmq::message_t createMsg(const uint16_t regAddress, const uint16_t regData);
    void respond(Response& res, const int errcode, const string& errmsg);
};

}//namespace pcs
}//namespace ems
