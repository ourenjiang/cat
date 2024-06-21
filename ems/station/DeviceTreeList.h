#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "Model.h"

namespace ems
{
using namespace std;

class DeviceTreeList
{
public:
    DeviceTreeList();

    vector<byte> respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg);
private:
    // 数据发布
    void registerHttpInterfaces();
    void requestCallback(const httplib::Request &req, httplib::Response &res);

    std::shared_ptr<ZmqRequest> requester_;
};

}//namespace ems
