#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace xftg
{
using namespace std;

class WeekPlan
{
public:
    using Record = std::tuple<string, string, string, string, string, string, string>;
    using RecordWithName = std::tuple<string, string, string, string, string, string, string, string>;
    WeekPlan();
    void respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body) const;

    static std::optional<vector<RecordWithName>> getAllRecords();
    static vector<int> convertDayofWeekListfromString(const string& data);
    vector<byte> identity();
private:
    // 数据发布
    void registerHttpInterfaces();
    void requestCallbackPost(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res);
    void requestCallbackDelete(const httplib::Request &req, httplib::Response &res);
    void requestCallbackPut(const httplib::Request &req, httplib::Response &res);

    std::optional<string> createTable();
    string createDayOfWeekListString(const vector<int>& dayOfWeekList);
    bool insertIntoDefaultRecord();
    std::optional<Record> getRecord(const string& name);

    zmq::socket_t dealer_;
    const string identity_;
    const string deleteSubtitle_;
    const string putSubtitle_;
    const string postSubtitle_;
};

}//namespace xftg
}//namespace ems
