#pragma once
#include "utils/HttpWrapper.h"
#include "ems/station/Model.h"
#include "zmq.hpp"

namespace ems
{
namespace bau
{
using namespace std;
using namespace httplib;

class Setting
{
public:
    Setting();
    void requestCallbackPowerOff(const httplib::Request &req, httplib::Response &res,
                                shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    void requestCallbackQuickStartup(const httplib::Request &req, httplib::Response &res,
                                shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    void requestCallbackSetBcuRelay(const httplib::Request &req, httplib::Response &res,
                                shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
private:
    zmq::message_t createMsg(const uint16_t regAddress, const uint16_t regData);
    void respond(Response& res, const int errcode, const string& errmsg);
};

}//namespace bau
}//namespace ems
