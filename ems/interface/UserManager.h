#pragma once
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "zmq.hpp"

namespace ems
{
using namespace std;
using namespace httplib;

class UserManager
{
public:
    UserManager();
    static void createDefaultRecord();

    void getNameListCallback(const Request&, Response&, shared_ptr<zmq::socket_t> stationDealer);
    void getAllCallback(const Request&, Response&, shared_ptr<zmq::socket_t> stationDealer);
    void getCallback(const Request&, Response&, shared_ptr<zmq::socket_t> stationDealer);
    void postCallback(const Request&, Response&, shared_ptr<zmq::socket_t> stationDealer);
    void putCallback(const Request&, Response&, shared_ptr<zmq::socket_t> stationDealer);
    void deleteCallback(const Request&, Response&, shared_ptr<zmq::socket_t> stationDealer);
private:
    log4cpp::Category& log_;
};
}//namespace ems
