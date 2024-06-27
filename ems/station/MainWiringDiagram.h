#pragma once
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "Model.h"

namespace ems
{
using namespace std;

class MainWiringDiagram
{
public:
    MainWiringDiagram();

    // 1.1 实时功率
    static Json::Value get_realPower(const StationInfo& stationInfo);

    // 1.2 累计电量信息
    static Json::Value get_capacitySum(const bau::BauStatusSummary& bauStatusSummary);

    // 1.2.1 累计电量信息:BMS
    static Json::Value get_capacitySum_bms(const bau::BauStatusSummary& bauStatusSummary);
    
    // 1.2.2 累计电量信息:光伏
    static Json::Value get_capacitySum_pv();
    
    // 1.3 环境信息
    static Json::Value get_env();
    
    // 1.4 模式
    static Json::Value get_mode();
    // 1.4.2 策略模式
    static string get_mode_strategy();
    
    // 1.5 电表
    static Json::Value get_meter();
    
    // 1.6 储能系统
    static Json::Value get_storageSystem(const bau::BauStatusSummary& bauStatusSummary);
    
    // 1.7 光伏系统
    static Json::Value get_pvSystem();
    
    // 1.8 负载系统
    static Json::Value get_loadSystem();
    
    // 1.9 环境管理
    static Json::Value get_envManage();
    
    // 1.10 BMS信息
    static Json::Value get_bms(const bau::BauStatusSummary& bauStatusSummary);
    
    // 1.11 告警统计
    static Json::Value get_warning(const bau::BauStatusSummary& bauStatusSummary);
    
    // 1.11.1 告警统计：按设备
    static Json::Value get_warning_byDevice(const bau::BauStatusSummary& bauStatusSummary);
    
    // 1.11.1 告警统计：按设备
    static Json::Value get_warning_byGrade(const bau::BauStatusSummary& bauStatusSummary);
    
    // 1.12 系统极值
    static Json::Value get_peak(const bau::BauStatusSummary& bauStatusSummary);

    void respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body);
    vector<byte> identity();
private:
    void registerAllInterfaces();
    void requestCallback(const httplib::Request &req, httplib::Response &res);
    static string convertCellAddrFormat(const uint16_t);

    const string identity_;
    zmq::socket_t dealer_;
    log4cpp::Category& log_;
};
}//namespace ems
