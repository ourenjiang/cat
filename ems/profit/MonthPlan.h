#pragma once
#include "utils/HttpWrapper.h"
#include <string>
#include <optional>
#include <tuple>
#include "ems/station/Model.h"
#include "zmq.hpp"

namespace ems
{
namespace electricity_price
{
using namespace std;

class MonthPlan
{
public:
    static void createTable();
    static void insertIntoDefaultRecord();

    using Record = tuple<string, string, string>;
    using RecordWithName = tuple<string, string, string, string>;
    static void requestCallbackGet(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackGetAll(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackPost(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackDelete(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    static void requestCallbackPut(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
private:

    static void deleteRecord(const string& name);
    static void modifyRecord(const string& name, const string& monthNo,
                        const string& dayPlanName, const string& typeListName);

    static optional<Record> getRecord(const string& name);
    static optional<vector<RecordWithName>> getAllRecord();
};

}//namespace electricity_price
}//namespace ems
