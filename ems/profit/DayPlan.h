#pragma once
#include "utils/HttpWrapper.h"
#include <vector>
#include <tuple>
#include <optional>
#include "zmq.hpp"
#include "ems/station/Model.h"

namespace ems
{
namespace electricity_price
{
using namespace std;

class DayPlan
{
public:
    static void createTable();
    static void insertIntoDefaultRecord();

    using Record = std::tuple<string, string, string, string>;
    using RecordList = vector<Record>;
    using NameList = vector<string>;
    static void requestCallbackGet(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackPost(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackDelete(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackPut(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
private:
    static void insertRecord(const string& name,
                        const string& durationName, const string& durationType,
                        const string& durationBegin, const string& durationEnd);
};

}//namespace electricity_price
}//namespace ems
