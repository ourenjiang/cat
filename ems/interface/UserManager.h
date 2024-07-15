#pragma once
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "zmq.hpp"
#include "sqlite_modern_cpp.h"

namespace ems
{
using namespace std;
using namespace httplib;

class UserManager
{
public:
    UserManager();
    static void createTable();
    static void insertIntoDefaultRecord();

    void getNameListCallback(const Request&, Response&,
        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    void getAllCallback(const Request&, Response&,
        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    void getCallback(const Request&, Response&,
        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    void postCallback(const Request&, Response&,
        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    void putCallback(const Request&, Response&,
        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    void deleteCallback(const Request&, Response&,
        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
private:
    log4cpp::Category& log_;
};
}//namespace ems
