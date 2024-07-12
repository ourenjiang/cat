#include "Processor.h"
#include "boost/assert.hpp"
#include <optional>
#include "ems/base/HistoryWarning.h"
#include "utils/Miscellaneous.h"
#include "ems/base/StationInfo.h"

using namespace std;
using namespace ems;

Processor::Processor(std::shared_ptr<zmq::socket_t> stationDealer)
    : stationDealer_(stationDealer)
{
    stationDealer_->set(zmq::sockopt::rcvtimeo, 1000);
}

Processor::~Processor()
{
    if(loopThread_.joinable()) loopThread_.join();
}

void Processor::start()
{
    loopThread_ = std::thread([&]{
    while(true){
        doWork();
    }});
}

void Processor::doWork()
{
    zmq::message_t msg;
    (void)stationDealer_->recv(msg);

    auto stationInfo = base::getStationInfo(stationDealer_);

    // 生成历史告警记录
    handleHistoryWarning(stationInfoBefore_, stationInfo);
    stationInfoBefore_ = stationInfo;// 覆盖旧数据
}

void Processor::handleHistoryWarning(const StationInfo& beforeInfo, const StationInfo& currentInfo)
{
    handleBauHistoryWarning(beforeInfo.bauMap_, currentInfo.bauMap_);
}

void Processor::handleBauHistoryWarning(const map<int, bau::BauInfo>& beforeInfo, const map<int, bau::BauInfo>& currentInfo)
{
    for(auto& currentBauInfo: currentInfo){
        auto result = beforeInfo.find(currentBauInfo.first);
        if(result != beforeInfo.end()){
            bauHistoryWarning_.handleBranch(currentBauInfo.first,  result->second, currentBauInfo.second);
        }
    }
}
