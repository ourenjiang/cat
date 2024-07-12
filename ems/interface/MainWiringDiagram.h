#pragma once
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "zmq.hpp"
#include "ems/station/Model.h"

namespace ems
{
using namespace std;

class MainWiringDiagram
{
public:
    MainWiringDiagram();
    void requestCallback(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer);
private:
    static string convertCellAddrFormat(const uint16_t);

    Json::Value get_realPower(const StationInfo& stationInfo);//实时功率
    Json::Value get_capacitySum(const bau::BauStatusSummary& bauStatusSummary);//累计电量信息
    Json::Value get_env();//环境信息
    Json::Value get_mode(const StationInfo& stationInfo);//模式
    Json::Value get_meter();//电表
    Json::Value get_storageSystem(const bau::BauStatusSummary& bauStatusSummary);//储能系统
    Json::Value get_pvSystem();//光伏系统
    Json::Value get_loadSystem();//负载系统
    Json::Value get_envManage();// 环境管理
    Json::Value get_bms(const bau::BauInfo& bauInfo);//BMS信息
    Json::Value get_warning(const bau::BauInfo& bauInfo, const pcs::PcsInfo& pcsInfo);//告警统计
    Json::Value get_peak(const bau::BauStatusSummary& bauStatusSummary);//系统极值

    log4cpp::Category& log_;
};
}//namespace ems
