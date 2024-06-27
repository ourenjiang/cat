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
    void respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body);
    vector<byte> identity();
private:
    string getPcsWarningAndFault(std::shared_ptr<StationInfo> stationInfo, const int branchIndex);

    // 数据发布
    void registerHttpInterfaces();
    void requestCallback(const httplib::Request &req, httplib::Response &res);

    const string dealerIdentity_;
    const string identity_;
    zmq::socket_t dealer_;
};

}//namespace bau
}//namespace ems
