#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "Model.h"

namespace ems
{
using namespace std;

class RealtimeWarning
{
public:
    RealtimeWarning();
    void respondCallbackGetWarningDeviceTree(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body);
    vector<byte> identity();
private:
    void registerHttpInterfaces(); // 数据发布
    void requestCallbackGetWarningDeviceTree(const httplib::Request &req, httplib::Response &res);

    const string identity_;
    zmq::socket_t dealer_;
};

}//namespace ems
