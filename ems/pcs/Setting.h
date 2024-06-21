#pragma once
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace pcs
{
using namespace std;

class Setting
{
public:
    Setting();
    vector<byte> respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const;
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallback(const httplib::Request &req, httplib::Response &res);

    std::unique_ptr<ZmqRequest> requester_;
};

}//namespace pcs
}//namespace ems
