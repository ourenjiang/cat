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
    
    void respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body) const;
    vector<byte> identity();
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

    zmq::socket_t dealer_;
    const string identity_;
    const string deleteSubtitle_;
    const string putSubtitle_;
    const string postSubtitle_;
};

}//namespace xftg
}//namespace ems