#pragma once
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace bau
{
using namespace std;

class Setting
{
public:
    Setting();
    vector<byte> respondCallbacPowerOff(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const;
    vector<byte> respondCallbacQuickStartup(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const;
    vector<byte> respondCallbacSetBcuRelay(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const;
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackPowerOff(const httplib::Request &req, httplib::Response &res);
    void requestCallbackQuickStartup(const httplib::Request &req, httplib::Response &res);
    void requestCallbackSetBcuRelay(const httplib::Request &req, httplib::Response &res);

    std::unique_ptr<ZmqRequest> stationRequester_;
    std::shared_ptr<ZmqRequest> bauRequester_;
};

}//namespace bau
}//namespace ems
