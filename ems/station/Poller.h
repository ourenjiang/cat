#pragma once
#include <vector>
#include <tuple>
#include <functional>
#include "utils/ZmqSubscribe.h"
#include "utils/ZmqRespond.h"
#include <map>
#include "Model.h"

namespace ems
{
using namespace std;
using PollerCallback = function<void(std::shared_ptr<StationInfo>&,
                        zmq::socket_t&, const vector<byte>&, const vector<byte>&, const vector<byte>&)>;
using RespondCallback = function<void(std::shared_ptr<StationInfo>&,
                        zmq::socket_t&, const vector<byte>&, const vector<byte>&, const vector<byte>&)>;
class Poller
{
public:
    Poller();
    void setStationInfo(std::shared_ptr<StationInfo> stationInfo){ stationInfo_ = stationInfo; }
    void addSubscriber(const string& address, const vector<string>& topicList, PollerCallback callback);
    void addRespondCallback(const vector<byte>& identity, RespondCallback callback);
    void doPoll();
private:

    void doRespond(std::shared_ptr<StationInfo>& stationInfo, zmq::socket_t& router, const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body);

    std::shared_ptr<StationInfo> stationInfo_;
    vector<zmq::socket_t> zmqSockets_;
    vector<zmq::pollitem_t> pollitems_;
    vector<PollerCallback> pollerCallbacks_;
    map<vector<byte>, RespondCallback> respondCallbacks_;
};
}//namespace ems
