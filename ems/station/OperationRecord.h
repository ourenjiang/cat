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
    
    vector<byte> respondCallbackGet(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    vector<byte> respondCallbackGetPageInfo(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetPageInfo(const httplib::Request &req, httplib::Response &res);

    std::string createTable();
    void insertIntoDefaultRecord();

    std::shared_ptr<ZmqRequest> requester_;
};

}//namespace ems
