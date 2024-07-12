#pragma once
#include <thread>
#include <memory>
#include "utils/Log4cppWrapper.h"
#include "msgpack.hpp"
#include "utils/ZmqNode.h"
#include "ems/bau/DeviceStatus.h"
#include "ems/station/Model.h"

namespace ems
{
namespace xftg
{
using namespace std;

struct DurationInfo
{
    string durationName;
    string durationBegin;
    string durationEnd;
    string controlType;
    int targetSoc;
    double targetPower;
    MSGPACK_DEFINE(durationBegin,
                   durationEnd, controlType, targetPower, targetSoc);
};

struct ProtectParams
{
    int socMax;
    int socMin;
    double transformerPowerMax;
    double powerStepSize;
    double chargePowerMax;
    double dischargePowerMax;
};

struct WeekPlanInfo
{
    string dayPlanDurationName;
    string dayPlanProtectName;
    vector<int> dayWhiteList;//[1-7]
    string validDateBegin;
    string validDateEnd;
    string priority;
    string bindSystem;
};

struct ExecuteParams
{
    // 电池实时状态
    double batteryCurrentVolt;  // 电压
    double batteryCurrentCur;   // 电流
    int batteryCurrentSoc;      // SOC

    // 电池推荐状态
    bau::BatteryStatus batteryChargeStatus;     // 充电状态
    bau::BatteryStatus batteryDischargeStatus;  // 放电状态

    // PCS实时状态
    double pcsCurrentSettingPower;  // 功率设置值
    double pcsCurrentOutputPower;   // 功率输出值
    pcs::RunStatus pcsCurrentStatus;// 状态：充电|放电|待机

    // 削峰填谷计划参数
    string userSuggestStatus;   // 充电 | 放电 | 待机
    double userSuggestPower;    // 输出功率
    int userSuggestSoc;         //目标SOC

    // 削峰填谷保护参数
    double allowChargePowerMax;     // 允许最大充电功率
    double allowDischargePowerMax;  // 允许最大放电功率
    int allowSocMax;                // 允许最大SOC
    int allowSocMin;                // 允许最小SOC

    uint32_t batteryBauThirdProtectStatus;
    uint32_t batteryBauFaultStatus;
    uint32_t batteryBingjiThirdProtectStatus;
    uint32_t batteryBingjiFaultStatus;
};

class StrategyXftg
{
public:
    StrategyXftg(std::shared_ptr<zmq::socket_t> stationDealer);
    ~StrategyXftg();
    void start();
private:
    void doWork();
    void doCommand(zmq::message_t& srcIdentity);
    void control(const StationInfo& stationInfo);
    void publish();
    zmq::message_t createMsg(const uint16_t regAddress, const uint16_t regData);

    void doUpdateInfo(const bau::BauInfo& bauInfo, const pcs::PcsInfo& pcsInfo);
    std::optional<vector<uint8_t>> doWork(const ExecuteParams& params);
    std::tuple<DurationInfo, ProtectParams> getTarget();

    std::optional<vector<uint8_t>> getActionWhenCharge(const ExecuteParams& params);
    std::optional<vector<uint8_t>> getActionWhenDischarge(const ExecuteParams& params);
    std::optional<vector<uint8_t>> getActionWhenStandby(const ExecuteParams& params);

    vector<uint8_t> getFramePowerOff();
    vector<uint8_t> getFramePowerOn();
    vector<uint8_t> getFrameChangeActivePower(const string& status, const double power);

    void loadAutoRun();
    void loadWeekPlanInfo();
    void loadDayPlanDurationInfo();
    void loadDayPlanProtectInfo();
    std::pair<string, WeekPlanInfo> getWeekPlan();
    DurationInfo getCurrentDurationInDayPlan(const string& name);
    ProtectParams getCurrentProtectParamsInWeekPlan(const string& name);

    // double getAdjustedPowerForSmoothOutput(const double rawPower, const double currentPower);
    void saveNewPowerHistory(const double newPower);

    // 充电状态校验
    vector<uint8_t> verifyChargeWithBatterySuggestPowerDown(const double pcsCurrentSettingPower);
    vector<uint8_t> verifyChargeWithBatterySuggestPowerStable(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc);
    vector<uint8_t> verifyChargeWithBatterySuggestPowerUp(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc);

    // 放电状态校验
    vector<uint8_t> verifyDischargeWithBatterySuggestPowerDown(const double pcsCurrentSettingPower);
    vector<uint8_t> verifyDischargeWithBatterySuggestPowerStable(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc);
    vector<uint8_t> verifyDischargeWithBatterySuggestPowerUp(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc);

    /*
        由于 底层模块所依赖的参数是阶段性更新的，所以将其缓存在上层模块中，
        当使用时，由上层模块以形参的形式统一传入；
        这样减少了阶段性的透传更新，
        同时，底层模块更专注于做'算法'逻辑而尽量少维护状态数据；
    */
    log4cpp::Category& log_;
    StrategyInfo strategyInfo_;
    bau::DeviceStatus bauStatus_;
    std::map<string, std::vector<DurationInfo>> dayPlanDurationMap_;
    std::map<string, ProtectParams> protectPrarmsMap_;
    std::map<string, WeekPlanInfo> weekPlanInfoMap_;
    std::shared_ptr<zmq::socket_t> stationDealer_;
    std::thread loopThread_;
};
}//namespace xftg
}//namespace ems
