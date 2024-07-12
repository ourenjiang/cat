#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "zmq.hpp"
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
    void requestCallback(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
private:
};

}//namespace bau
}//namespace ems
