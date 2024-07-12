#pragma once
#include "utils/HttpWrapper.h"
#include "zmq.hpp"
#include "ems/station/Model.h"

namespace ems
{
namespace xftg
{
using namespace std;

class Setting
{
public:
    static void createTable();
    static void insertIntoDefaultRecord();

    static optional<string> getRecord(const string& branchIndex);
    static void requestCallbackGet(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> stationDealer);
    static void requestCallbackPut(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> stationDealer);

};

}//namespace xftg
}//namespace ems
