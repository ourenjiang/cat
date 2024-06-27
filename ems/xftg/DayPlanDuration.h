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
    void respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body) const;
    static vector<Record> getRecord(const string& name);
    static std::optional<map<string, vector<Record>>> getAllRecord();
    vector<byte> identity();
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackPost(const httplib::Request &req, httplib::Response &res);
    void requestCallbackDelete(const httplib::Request &req, httplib::Response &res);
    void requestCallbackPut(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res);
    
    std::string createTable();
    bool insertIntoDefaultRecord();

    zmq::socket_t dealer_;
    const string identity_;
    const string deleteSubtitle_;
    const string putSubtitle_;
    const string postSubtitle_;
};

}//xftg
}//ems
