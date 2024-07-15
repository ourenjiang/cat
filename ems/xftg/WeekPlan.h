#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "zmq.hpp"
#include "ems/station/Model.h"

namespace ems
{
namespace xftg
{
using namespace std;

class WeekPlan
{
public:
    static void createTable();
    static void insertIntoDefaultRecord();

    using Record = std::tuple<string, string, string, string, string, string, string>;
    using RecordWithName = std::tuple<string, string, string, string, string, string, string, string>;
    static void requestCallbackPost(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackGet(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackDelete(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackPut(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);

    static std::optional<vector<RecordWithName>> getAllRecords();
    static vector<int> convertDayofWeekListfromString(const string& data);
private:

    static string createDayOfWeekListString(const vector<int>& dayOfWeekList);
    static std::optional<Record> getRecord(const string& name);
};

}//namespace xftg
}//namespace ems
