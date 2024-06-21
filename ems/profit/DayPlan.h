#pragma once
#include "utils/HttpWrapper.h"
#include <vector>
#include <tuple>
#include <optional>
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace electricity_price
{
using namespace std;

class DayPlan
{
public:
    using Record = std::tuple<string, string, string, string>;
    using RecordList = vector<Record>;
    using NameList = vector<string>;
    
    DayPlan();
    vector<byte> respondCallbackPost(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    vector<byte> respondCallbackDelete(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    vector<byte> respondCallbackPut(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackPost(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res);
    void requestCallbackDelete(const httplib::Request &req, httplib::Response &res);
    void requestCallbackPut(const httplib::Request &req, httplib::Response &res);

    std::string createTable() const;
    bool insertIntoDefaultRecord() const;
    void insertRecord(const string& name,
                        const string& durationName, const string& durationType,
                        const string& durationBegin, const string& durationEnd) const;
    
    std::unique_ptr<ZmqRequest> requester_;
};

}//namespace electricity_price
}//namespace ems
