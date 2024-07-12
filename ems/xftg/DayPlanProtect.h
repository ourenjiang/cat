#pragma once
#include "utils/HttpWrapper.h"
#include "zmq.hpp"
#include "ems/station/Model.h"

namespace ems
{
namespace xftg
{
using namespace std;

class DayPlanProtect
{
public:
    static void createTable();
    static void insertIntoDefaultRecord();

    using Record = tuple<string, string, string, string, string, string>;
    static void requestCallbackGet(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackPost(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackDelete(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackPut(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    
    static std::optional<map<string, Record>> getAllRecord();
private:
    static std::optional<Record> getRecord(const string& name);
};

}//namespace xftg
}//namespace ems