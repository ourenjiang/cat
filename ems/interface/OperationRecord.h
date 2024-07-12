#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "zmq.hpp"

namespace ems
{
using namespace std;

class OperationRecord
{
public:
    static void createTable();
    static void insertIntoDefaultRecord();

    using OperationRecordInfo = tuple<string, string, string, string, string>;
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
    void requestCallbackGetPageInfo(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
};

}//namespace ems
