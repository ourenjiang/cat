#pragma once
#include "utils/HttpWrapper.h"
#include <optional>
#include <tuple>
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace electricity_price
{
using namespace std;

class TypeList
{
public:
    using Record = tuple<string, string, string, string, string, string, string, string>;
    using NameList = vector<string>;
    
    TypeList();
    vector<byte> respondCallbackPost(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const;
    vector<byte> respondCallbackPut(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const;
    vector<byte> respondCallbackDelete(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const;
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackPost(const httplib::Request &req, httplib::Response &res);
    void requestCallbackPut(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res);
    void requestCallbackDelete(const httplib::Request &req, httplib::Response &res);

    std::string createTable();
    bool insertIntoDefaultRecord();

    Record getRecord(const string& name) const;

    std::unique_ptr<ZmqRequest> requester_;
};

}//namespace electorcity_price
}//namespace ems
