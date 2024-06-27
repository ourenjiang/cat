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
    void respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body) const;
    vector<byte> identity();
private:
    void registerHttpInterfaces();// 数据发布
    void requestCallback(const httplib::Request &req, httplib::Response &res);

    const string identity_;
    zmq::socket_t dealer_;
};

}//namespace pcs
}//namespace ems
