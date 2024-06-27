#pragma once
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "Model.h"

namespace ems
{
using namespace std;

class PublicInfo
{
public:
    PublicInfo();
    void respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body);
    vector<byte> dealerIdentity();
private:
    void registerAllInterfaces();
    void requestCallback(const httplib::Request &req, httplib::Response &res);

    log4cpp::Category& log_;
    const string identity_;
    zmq::socket_t dealer_;
};
}//namespace ems
