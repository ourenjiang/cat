#pragma once
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace xftg
{
using namespace std;

class DayPlanProtect
{
public:
    using Record = tuple<string, string, string, string, string, string>;
    DayPlanProtect();
    static std::optional<map<string, Record>> getAllRecord();
    
    vector<byte> respondCallbackDelete(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    vector<byte> respondCallbackPost(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    vector<byte> respondCallbackPut(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
private:
    // 数据发布
    void registerHttpInterfaces();
    void requestCallbackPost(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res);
    void requestCallbackDelete(const httplib::Request &req, httplib::Response &res);
    void requestCallbackPut(const httplib::Request &req, httplib::Response &res);

    string createTable();
    bool insertIntoDefaultRecord();

    std::optional<Record> getRecord(const string& name);

    std::unique_ptr<ZmqRequest> requester_;
};

}//namespace xftg
}//namespace ems