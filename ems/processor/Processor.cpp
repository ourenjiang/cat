#include "Processor.h"
#include "boost/assert.hpp"
#include <optional>
#include "ems/base/HistoryWarning.h"
#include "utils/Miscellaneous.h"
#include "ems/base/StationInfo.h"

using namespace std;
using namespace ems;

Processor::Processor(std::shared_ptr<zmq::socket_t> dataDealer)
    : timer_(1000)
    , dataDealer_(dataDealer)
{
    timer_.setTimeoutCallback(std::bind(&Processor::doWork, this));
    // dataDealer_->set(zmq::sockopt::rcvtimeo, 1000);
}

Processor::~Processor()
{
    // if(loopThread_.joinable()) loopThread_.join();
}

void Processor::start()
{
    // loopThread_ = std::thread([&]{
    // while(true){
    //     doWork();
    // }});
    timer_.start();
}

void Processor::doWork()
{
    auto stationInfo = base::getStationInfo(dataDealer_);

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
