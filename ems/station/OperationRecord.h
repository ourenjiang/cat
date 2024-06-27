#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "Model.h"

namespace ems
{
using namespace std;

class OperationRecord
{
public:
    using OperationRecordInfo = tuple<string, string, string, string, string>;
    OperationRecord();
    static void insertRecord(const string& status, const string& content, const string& type, const string& username);
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetPageInfo(const httplib::Request &req, httplib::Response &res);

    std::string createTable();
    void insertIntoDefaultRecord();
};

}//namespace ems
