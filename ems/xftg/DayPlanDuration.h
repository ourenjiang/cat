#pragma once
#include <string>
#include <vector>
#include <optional>
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace xftg
{
using namespace std;

class DayPlanDuration
{
public:
    using Record = tuple<string, string, string, string, string, string>;
    DayPlanDuration();
    vector<byte> respondCallbackDelete(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    vector<byte> respondCallbackPut(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    vector<byte> respondCallbackPost(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    static vector<Record> getRecord(const string& name);
    static std::optional<map<string, vector<Record>>> getAllRecord();
private:
    void registerHttpInterfaces();// 数据发布
    optional<string> request(const string& requestMessage);
    void requestCallbackPost(const httplib::Request &req, httplib::Response &res);
    void requestCallbackDelete(const httplib::Request &req, httplib::Response &res);
    void requestCallbackPut(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res);
    
    std::string createTable();
    bool insertIntoDefaultRecord();
    
    std::unique_ptr<ZmqRequest> requester_;
};

}//xftg
}//ems
