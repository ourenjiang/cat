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
    void respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body);
    void respondCallbackBau(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg, zmq::socket_t& router, const vector<byte>& identity);
    void respondCallbackBcu(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg, zmq::socket_t& router, const vector<byte>& identity);
    vector<byte> identity();
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackBau(const httplib::Request &req, httplib::Response &res);
    void requestCallbackBcu(const httplib::Request &req, httplib::Response &res);

    const string bauSubtitle_;
    const string bcuSubtitle_;
    const string identity_;
    zmq::socket_t dealer_;
};

}//namespace bau
}//namespace ems
