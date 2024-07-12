/**
 * 加工处理总入口。
*/
#pragma once
#include "utils/ZmqNode.h"
#include <thread>
#include "ems/station/Model.h"
#include <bitset>
#include <unordered_map>
#include "BauHistoryWarning.h"

namespace ems
{
using namespace std;
using namespace std::placeholders;
using HistoryRecord = tuple<string, string, string, string, string, string>;

class Processor
{
public:
    Processor(std::shared_ptr<zmq::socket_t> stationDealer);
    ~Processor();
    void start();
private:
    void doWork();
    void handleHistoryWarning(const StationInfo& beforeInfo, const StationInfo& currentInfo);
    void handleBauHistoryWarning(const map<int, bau::BauInfo>& beforeInfo, const map<int, bau::BauInfo>& currentInfo);
    StationInfo stationInfoBefore_;
    shared_ptr<zmq::socket_t> stationDealer_;
    processed::BauHistoryWarning bauHistoryWarning_;
    std::thread loopThread_;
};

}//namespace ems
