#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace pcs
{
using namespace std;

class RealtimeWarning
{
public:
    RealtimeWarning();
    vector<byte> respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg);
private:
    string getPcsWarningAndFault(std::shared_ptr<StationInfo> stationInfo, const int branchIndex);

    // 数据发布
    void registerHttpInterfaces();
    void requestCallback(const httplib::Request &req, httplib::Response &res);

    std::shared_ptr<ZmqRequest> requester_;
};

}//namespace bau
}//namespace ems
