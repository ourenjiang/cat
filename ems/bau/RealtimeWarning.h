#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace bau
{
using namespace std;

class RealtimeWarning
{
public:
    RealtimeWarning();
    vector<byte> respondCallbackBau(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg);
    vector<byte> respondCallbackBcu(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg);
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackBau(const httplib::Request &req, httplib::Response &res);
    void requestCallbackBcu(const httplib::Request &req, httplib::Response &res);

    std::shared_ptr<ZmqRequest> requester_;
};

}//namespace bau
}//namespace ems
