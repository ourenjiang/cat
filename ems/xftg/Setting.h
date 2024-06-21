#pragma once
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace xftg
{
using namespace std;

class Setting
{
public:
    Setting();
    vector<byte> respondPutCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    static optional<string> getRecord(const string& branchIndex);
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackPut(const httplib::Request &req, httplib::Response &res);

    string createTable();
    void insertIntoDefaultRecord();

    std::unique_ptr<ZmqRequest> requester_;
};

}//namespace xftg
}//namespace ems
