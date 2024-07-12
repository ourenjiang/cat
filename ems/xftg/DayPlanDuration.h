#pragma once
#include <string>
#include <vector>
#include <optional>
#include "utils/HttpWrapper.h"
#include "zmq.hpp"
#include "ems/station/Model.h"
#include "json/json.h"

namespace ems
{
namespace xftg
{
using namespace std;

class DayPlanDuration
{
public:
    static void createTable();
    static void insertIntoDefaultRecord();

    using Record = tuple<string, string, string, string, string, string>;
    static void requestCallbackGet(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackPost(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackPut(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackDelete(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);

    static vector<Record> getRecord(const string& name);
    static std::optional<map<string, vector<Record>>> getAllRecord();
private:
    static void notifyStationReload(shared_ptr<zmq::socket_t> stationDealer);
    static pair<string, vector<Record>> parseRecordFromRequestBody(const Json::Value& root);
};

}//xftg
}//ems
