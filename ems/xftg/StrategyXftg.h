#pragma once
#include <thread>
#include <memory>
#include "utils/YamlcppWrapper.h"
#include "utils/Log4cppWrapper.h"
#include "ems/pcs/Controler.h"
#include "ems/bau/Controler.h"
#include "msgpack.hpp"
#include "utils/ZmqSubscribe.h"

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
    double batteryCurrentVolt;
    double batteryCurrentCur;
    double batteryCurrentPower;
    int batteryCurrentSoc;

    double batterySuggestChargeVolt;
    double batterySuggestChargeCur;
    double batterySuggestDischargeVolt;
    double batterySuggestDischargeCur;

    double pcsCurrentSettingPower;
    double pcsCurrentOutputPower;
    string pcsCurrentStatus;

    string userSuggestStatus;// 充电 | 放电 | 待机
    double userSuggestPower;
    int userSuggestSoc;

    double allowChargePowerMax;
    double allowDischargePowerMax;
    int allowSocMax;
    int allowSocMin;

    uint32_t batteryBauThirdProtectStatus;
    uint32_t batteryBauFaultStatus;
    uint32_t batteryBingjiThirdProtectStatus;
    uint32_t batteryBingjiFaultStatus;

    MSGPACK_DEFINE(batteryCurrentVolt, batteryCurrentCur, batteryCurrentPower, batteryCurrentSoc,
                   batterySuggestChargeVolt, batterySuggestChargeCur, batterySuggestDischargeVolt, batterySuggestDischargeCur,
                   pcsCurrentSettingPower, pcsCurrentOutputPower, pcsCurrentStatus,
                   userSuggestStatus, userSuggestPower, userSuggestSoc,
                   allowChargePowerMax, allowDischargePowerMax, allowSocMax, allowSocMin,
                   batteryBauThirdProtectStatus, batteryBauFaultStatus, batteryBingjiThirdProtectStatus, batteryBingjiFaultStatus);
};

class StrategyXftg
{
public:
    StrategyXftg();
    ~StrategyXftg();
    void setWeekPlanInfo(const std::map<string, WeekPlanInfo>& weekPlanInfo);
    void setDurationInfo(const pair<string, std::vector<DurationInfo>>& durationInfo);
    string getDayPlanName();
    std::tuple<DurationInfo, ProtectParams> getTarget();
    void doWork(const ExecuteParams& params);
    void setAutoRun(const bool flag){ autoRun_ = flag; }
    bool getAutoRun(){ return autoRun_; }
    void start();
private:
    void loadWeekPlanInfo();
    void loadDayPlanDurationInfo();
    void loadDayPlanProtectInfo();
    std::pair<string, WeekPlanInfo> getWeekPlan();
    DurationInfo getCurrentDurationInDayPlan(const string& name);
    ProtectParams getCurrentProtectParamsInWeekPlan(const string& name);

    // double getAdjustedPowerForSmoothOutput(const double rawPower, const double currentPower);
    void saveNewPowerHistory(const double newPower);

    // 充电状态校验
    void verifyChargeWithBatterySuggestPowerDown(const double pcsCurrentSettingPower);
    void verifyChargeWithBatterySuggestPowerStable(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc);
    void verifyChargeWithBatterySuggestPowerUp(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc);

    // 放电状态校验
    void verifyDischargeWithBatterySuggestPowerDown(const double pcsCurrentSettingPower);
    void verifyDischargeWithBatterySuggestPowerStable(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc);
    void verifyDischargeWithBatterySuggestPowerUp(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc);

    /*
        由于 底层模块所依赖的参数是阶段性更新的，所以将其缓存在上层模块中，
        当使用时，由上层模块以形参的形式统一传入；
        这样减少了阶段性的透传更新，
        同时，底层模块更专注于做'算法'逻辑而尽量少维护状态数据；
    */
    log4cpp::Category& log_;
    std::shared_ptr<pcs::Controler> pcsControler_;
    std::shared_ptr<bau::Controler> bauControler_;
    std::map<string, std::vector<DurationInfo>> dayPlanDurationMap_;
    std::map<string, ProtectParams> protectPrarmsMap_;
    std::map<string, WeekPlanInfo> weekPlanInfoMap_;
    bool autoRun_;
    zmq::socket_t subscriber_;
    std::thread loopThread_;
};
}//namespace xftg
}//namespace ems
