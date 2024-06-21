#pragma once
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "Model.h"

namespace ems
{
using namespace std;

class StorageEnergySystem
{
public:
    StorageEnergySystem();
    
    // 1 实时趋势
    static Json::Value get_trend(const StationInfo& stationInfo);
    // 1.1 实时趋势:SOC
    static Json::Value get_trend_soc(const StationInfo& stationInfo);
    // 1.2 实时趋势:电流
    static Json::Value get_trend_cur(const StationInfo& stationInfo);
    // 1.3 实时趋势:功率
    static Json::Value get_trend_power(const StationInfo& stationInfo);
    // 1.2 中央顶部数据区
    static Json::Value get_centralTopArea(const bau::BauStatusSummary& bauStatusSummary);
    // 1.3 储能分支
    static Json::Value get_branch(const StationInfo& stationInfo);
    // 1.3.1 储能分支:PCS
    static Json::Value get_branch_pcs(const pcs::_0406_0460_Summary& summary);
    // 1.3.2 储能分支:bau
    static Json::Value get_branch_bau(const bau::BauStatusSummary& bauStatusSummary);
    // 1.4 告警信息
    static Json::Value get_warning(const bau::BauStatusSummary& bauStatusSummary, const pcs::_0406_0460_Summary& pcs_0406_0460_summary);
    static size_t getWarningNumOfBau(const bau::BauStatusSummary& bauStatusSummary);
    static size_t getWarningNumOfPcs(const pcs::_0406_0460_Summary& summary);
    
    // 1.5 系统极值
    static Json::Value get_peak(const bau::BauStatusSummary& bauStatusSummary);
    vector<byte> respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg);

private:
    void registerAllInterfaces();
    void requestCallback(const httplib::Request &req, httplib::Response &res);
    pair<bool, string> handleGetHttp();
    static string convertCellAddrFormat(const uint16_t);

    log4cpp::Category& log_;
    std::unique_ptr<ZmqRequest> requester_;
};
}//namespace interface_storage_energy_system
