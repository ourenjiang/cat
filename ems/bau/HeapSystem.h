#pragma once
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "ems/bau/Model.h"
#include "ems/station/Model.h"

namespace ems
{
namespace bau
{
using namespace std;

class HeapSystem
{
public:
    HeapSystem();
    vector<byte> respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const ;
private:
    void registerAllInterfaces();
    void requestCallback(const httplib::Request &req, httplib::Response &res);

    string convertCellAddrFormat(const uint16_t) const;
    Json::Value get_base(const BauStatusSummary&) const;//基础信息
    Json::Value get_powerEnvSystem() const;//动环信息
    Json::Value get_bcu(const map<int, BcuInfo>&) const;
    Json::Value get_bcu(const BcuInfo&) const;
    Json::Value get_warning(const BauStatusSummary&) const;//告警信息
    Json::Value get_peak(const BauStatusSummary&) const;//系统极值
    
    log4cpp::Category& log_;
    std::unique_ptr<ZmqRequest> requester_;
};
}//namespace bau
}//namespace ems
