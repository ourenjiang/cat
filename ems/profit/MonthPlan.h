#pragma once
#include "utils/HttpWrapper.h"
#include <string>
#include <optional>
#include <tuple>
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace electricity_price
{
using namespace std;

class MonthPlan
{
public:
    using Record = tuple<string, string, string>;
    using RecordWithName = tuple<string, string, string, string>;

    MonthPlan();
    vector<byte> respondCallbackPost(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    vector<byte> respondCallbackDelete(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    vector<byte> respondCallbackPut(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
private:
    // 数据发布
    void registerHttpInterfaces();
    void requestCallbackPost(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetAll(const httplib::Request &req, httplib::Response &res);
    void requestCallbackDelete(const httplib::Request &req, httplib::Response &res);
    void requestCallbackPut(const httplib::Request &req, httplib::Response &res);

    std::string createTable();
    bool insertIntoDefaultRecord();
    bool deleteRecord(const string& name);
    bool modifyRecord(const string& name, const string& monthNo,
                        const string& dayPlanName, const string& typeListName);

    optional<Record> getRecord(const string& name);
    optional<vector<RecordWithName>> getAllRecord();

    std::unique_ptr<ZmqRequest> requester_;
};

}//namespace electricity_price
}//namespace ems
