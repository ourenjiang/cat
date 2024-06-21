#pragma once
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
using namespace std;

class Profit
{
public:
    Profit();
    vector<byte> respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg);
private:
    void registerAllInterfaces();
    void requestCallback(const httplib::Request &req, httplib::Response &res);

    log4cpp::Category& log_;
    std::unique_ptr<ZmqRequest> requester_;
};
}//namespace interface
