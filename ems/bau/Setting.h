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
    void respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body);
    void respondCallbacPowerOff(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                                const vector<byte>& identity, const vector<byte>& body);
    void respondCallbacQuickStartup(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                                    const vector<byte>& identity, const vector<byte>& body);
    void respondCallbacSetBcuRelay(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                                    const vector<byte>& identity, const vector<byte>& body);
    vector<byte> identity();
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallbackPowerOff(const httplib::Request &req, httplib::Response &res);
    void requestCallbackQuickStartup(const httplib::Request &req, httplib::Response &res);
    void requestCallbackSetBcuRelay(const httplib::Request &req, httplib::Response &res);

    const string identity_;
    const string powerOffSubtitle_;
    const string quickStartupSubtitle_;
    const string setBcuRelaySubtitle_;
    zmq::socket_t stationDealer_;
    zmq::socket_t bauDealer_;
};

}//namespace bau
}//namespace ems
