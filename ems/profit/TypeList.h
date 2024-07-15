#pragma once
#include "utils/HttpWrapper.h"
#include <optional>
#include <tuple>
#include "ems/station/Model.h"
#include "zmq.hpp"

namespace ems
{
namespace electricity_price
{
using namespace std;

class TypeList
{
public:
    static void createTable();
    static void insertIntoDefaultRecord();

    using Record = tuple<string, string, string, string, string, string, string, string>;
    using NameList = vector<string>;
    static void requestCallbackPost(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackPut(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackGet(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackDelete(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
private:
    static Record getRecord(const string& name);
};

}//namespace electorcity_price
}//namespace ems
