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

class Poller
{
public:
    Poller();
    void setStationInfo(std::shared_ptr<StationInfo> stationInfo){ stationInfo_ = stationInfo; }
    void addSubscriber(const string& address, const vector<string>& topicList, 
                        function<void(const string&)> readCallback);
    void doPoll();
    void addRespondCallback(const string& key,
                            function<vector<byte> (std::shared_ptr<StationInfo>, const vector<byte>&)> callback);
private:

    void doRespond();

    std::shared_ptr<StationInfo> stationInfo_;
    vector<tuple<ZmqSubscribe, function<void(const string&)>>> handlers_;
    vector<zmq::pollitem_t> pollitems_;

    unique_ptr<ZmqRespond> responser_;
    int responserPollIndex_;
    map<vector<byte>, function<vector<byte> (std::shared_ptr<StationInfo>, const vector<byte>&)>> respondCallbackMap_;
};
}//namespace ems
