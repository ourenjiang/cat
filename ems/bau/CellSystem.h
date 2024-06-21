#pragma once
#include <tuple>
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "ems/bau/BmuPoller.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace bau
{
using namespace std;

class CellSystem
{
public:
    CellSystem();
    vector<byte> respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
private:
    void registerAllInterfaces();
    void requestCallback(const httplib::Request &req, httplib::Response &res);

    static Json::Value get_statistic(const CellvoltSummary&, const CelltemSummary&);//统计数据
    static Json::Value get_cellvolt(const CellvoltSummary&);//单体电压
    static Json::Value get_celltem(const CelltemSummary&);//单体温度
    static Json::Value get_terminaltem(const CelltemSummary&);//端子温度

    log4cpp::Category& log_;
    std::unique_ptr<ZmqRequest> requester_;
};
}//namespace bau
}//namespace ems
