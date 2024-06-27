#include "StrategyXftg.h"
#include <iostream>
#include <filesystem> // since c++17
#include <regex>
#include "boost/assert.hpp"
#include "DayPlanDuration.h"
#include "DayPlanProtect.h"
#include "WeekPlan.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"

using namespace std;
using namespace std::chrono;
using namespace boost;
using namespace ems;
using namespace ems::xftg;

StrategyXftg::StrategyXftg()
    : log_(ems::Log4cppWrapper::getLogger(0))
    , pcsControler_(make_shared<pcs::Controler>())
    , bauControler_(make_shared<bau::Controler>())
    , subscriber_(miscellaneous::createZmqSocket(zmq::socket_type::sub))
{
    loadWeekPlanInfo();
    loadDayPlanDurationInfo();
    loadDayPlanProtectInfo();

    subscriber_.set(zmq::sockopt::subscribe, "ExecStrategy");
    subscriber_.connect("tcp://localhost:9100");
}

StrategyXftg::~StrategyXftg()
{
    if(loopThread_.joinable()){
        loopThread_.join();
    }
}

void StrategyXftg::loadWeekPlanInfo()
{
    // 从数据库表加载周计划
    const auto records = WeekPlan::getAllRecords();
    BOOST_ASSERT(records.has_value());

    for(const auto& item : records.value()){

        WeekPlanInfo info;
        info.dayPlanDurationName = std::get<1>(item);
        info.dayPlanProtectName = std::get<2>(item);
        info.dayWhiteList = WeekPlan::convertDayofWeekListfromString(std::get<3>(item));
        info.validDateBegin = std::get<4>(item);
        info.validDateEnd = std::get<5>(item);
        info.priority = std::get<6>(item);
        info.bindSystem = std::get<7>(item);

        weekPlanInfoMap_.emplace(std::get<0>(item), info);
    }
}

void StrategyXftg::loadDayPlanDurationInfo()
{
    // 从数据库加载日计划
    const auto recordsMap = DayPlanDuration::getAllRecord();
    BOOST_ASSERT(recordsMap.has_value());

    for(const auto& records : recordsMap.value()){

        auto& durationInfoList = dayPlanDurationMap_[records.first];

        for(const auto& record : records.second){

            DurationInfo info;
            info.durationName = std::get<0>(record);
            info.durationBegin = std::get<1>(record);
            info.durationEnd = std::get<2>(record);
            info.controlType = std::get<3>(record);
            {
                std::regex pattern(R"((\d+)%)");// 匹配类似100%中的数值
                std::smatch matches;
                const bool searchResult = std::regex_search(std::get<4>(record), matches, pattern);
                BOOST_ASSERT(searchResult);
                info.targetSoc = std::stoi(matches[1].str());
            }
            {
                // std::regex pattern("[-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?kW");// 匹配类似100%中的数值
                // std::smatch matches;
                // const bool searchResult = std::regex_search(std::get<5>(record), matches, pattern);
                // BOOST_ASSERT(searchResult);
                // const string testString = matches[1].str();

                string content = std::get<5>(record);
                content.pop_back();
                content.pop_back();
                info.targetPower = std::stod(content);
            }
            durationInfoList.emplace_back(info);
        }
    }
}

void StrategyXftg::loadDayPlanProtectInfo()
{
    // 从数据库加载保护参数
    const auto recordsMap = DayPlanProtect::getAllRecord();
    BOOST_ASSERT(recordsMap.has_value());

    for(const auto& item : recordsMap.value()){

        ProtectParams info;
        info.socMax = std::stoi(std::get<0>(item.second));
        info.socMin = std::stoi(std::get<1>(item.second));
        info.transformerPowerMax = std::stod(std::get<2>(item.second));
        info.powerStepSize = std::stod(std::get<3>(item.second));
        info.chargePowerMax = std::stod(std::get<4>(item.second));
        info.dischargePowerMax = std::stod(std::get<5>(item.second));

        protectPrarmsMap_.emplace(std::get<0>(item), info);
    }
}

void StrategyXftg::start()
{
    // zmq::pollitem_t item{ subscriber_->socket(), 0, ZMQ_POLLIN, 0 };

    loopThread_ = thread([&]{
    while(true)
    {
        zmq::message_t topic;
        zmq::message_t subtitle;
        zmq::message_t body;
        (void)subscriber_.recv(topic);
        (void)subscriber_.recv(subtitle);
        (void)subscriber_.recv(body);

        // string rBuffer;
        // subscriber_->recv(rBuffer);

        // 这里需要解析数据;
        // const string topic{ "ExecStrategy" };

        ExecuteParams params;
        const bool unserializedResult = msgpackWrapper::unpack(body.data(), body.size(), params);
        BOOST_ASSERT(unserializedResult);

        doWork(params);
    }});
}

void StrategyXftg::doWork(const ExecuteParams& params)
{
    {
        //      由于后续应该根据PCS的功率设定值判断整个分支的充放电状态，
        //      因此，在前期检查得到告警、故障或用户指令时，应该及时将功率设定值置为0

        // 1，自动运行开关检查
        if(!this->getAutoRun()){
            return;
        }

        // 2, 告警、故障位检查
        // const std::bitset<32> batteryBauThirdProtectStatus = params.batteryBauThirdProtectStatus;
        // const std::bitset<32> batteryBauFaultStatus = params.batteryBauFaultStatus;
        // const std::bitset<32> batteryBingjiThirdProtectStatus = params.batteryBingjiThirdProtectStatus;
        // const std::bitset<32> batteryBingjiFaultStatus = params.batteryBingjiFaultStatus;
        // if(batteryBauThirdProtectStatus.count() || batteryBauFaultStatus.count()
        //     || batteryBingjiThirdProtectStatus.count() || batteryBingjiFaultStatus.count()){
        if(!bauControler_->verifyNormal(params.batteryBauThirdProtectStatus, params.batteryBauFaultStatus,
                                        params.batteryBingjiThirdProtectStatus, params.batteryBingjiFaultStatus)){
            // 告警、故障产生，需要关机
            if(params.pcsCurrentStatus != "PowerOff"){
                pcsControler_->setPowerOff();
            }
            return;
        }
        else if(params.pcsCurrentStatus == "PowerOff"){
            // 需要开机
            pcsControler_->setPowerOn();
            return;// 本轮先开机，下轮再执行正常工作
        }
    }

    // 当前充电
    if(params.pcsCurrentStatus == "charge")
    {
        // 禁止充电
        if(!bauControler_->verifyChargeAbility(params.batteryBauThirdProtectStatus, params.batteryBingjiThirdProtectStatus)){
            // 降低功率
            double newPower = params.pcsCurrentSettingPower;
            if(newPower > 1.0) newPower -= 1.0;
            else newPower = 0.0;
            pcsControler_->setActivePower("charge", newPower);
            return;
        }

        // 校验充电保护参数(功率);
        double userSuggestPower = params.userSuggestPower;
        if(userSuggestPower > params.allowChargePowerMax){
            userSuggestPower = params.allowChargePowerMax;
        }

        // 校验充电保护参数(SOC);
        int userSuggestSoc = params.userSuggestSoc;
        if(userSuggestSoc > params.allowSocMax){
            userSuggestSoc = params.allowSocMax;
        }

        // 获取电池推荐充电状态
        const auto batteryChargeStatus = bauControler_->getChargeStatus(params.batteryCurrentVolt, params.batteryCurrentCur,
                                                                    params.batterySuggestChargeVolt, params.batterySuggestChargeCur);

        // 电池推荐：降功率 => 降功率：结束
        if(batteryChargeStatus == bau::BatteryStatus::SuggestDown){
            verifyChargeWithBatterySuggestPowerDown(params.pcsCurrentSettingPower);
            return;
        }

        // 电池推荐：稳定输出
        if(batteryChargeStatus == bau::BatteryStatus::Stable){
            
            verifyChargeWithBatterySuggestPowerStable(params.userSuggestStatus,
                                                        userSuggestPower, params.pcsCurrentSettingPower,
                                                        userSuggestSoc, params.batteryCurrentSoc);
            return;
        }

        // 电池推荐：提高功率
        if(batteryChargeStatus == bau::BatteryStatus::SuggestUp){
            verifyChargeWithBatterySuggestPowerUp(params.userSuggestStatus,
                                                    userSuggestPower, params.pcsCurrentSettingPower,
                                                    userSuggestSoc, params.batteryCurrentSoc);
            return;
        }
        return;
    }

    // 当前放电
    if(params.pcsCurrentStatus == "discharge")
    {
        // 禁止放电
        if(!bauControler_->verifyDischargeAbility(params.batteryBauThirdProtectStatus, params.batteryBingjiThirdProtectStatus)){
            // 降低功率
            double newPower = params.pcsCurrentSettingPower;
            if(newPower > 1.0) newPower -= 1.0;
            else newPower = 0.0;
            pcsControler_->setActivePower("discharge", newPower);
            return;
        }

        // 校验放电保护参数(功率);
        double userSuggestPower = params.userSuggestPower;
        if(userSuggestPower > params.allowDischargePowerMax){
            userSuggestPower = params.allowDischargePowerMax;
        }

        // 校验放电保护参数(SOC);
        int userSuggestSoc = params.userSuggestSoc;
        if(userSuggestSoc < params.allowSocMin){
            userSuggestSoc = params.allowSocMin;
        }

        // 获取电池推荐充电状态
        const auto batteryDischargeStatus = bauControler_->getDischargeStatus(params.batteryCurrentVolt, params.batteryCurrentCur,
                                                                                params.batterySuggestDischargeVolt, params.batterySuggestDischargeCur);
        // 电池推荐：降功率 => 降功率：结束
        if(batteryDischargeStatus == bau::BatteryStatus::SuggestDown){
            verifyDischargeWithBatterySuggestPowerDown(params.pcsCurrentSettingPower);
            return;
        }

        // 电池推荐：稳定输出
        if(batteryDischargeStatus == bau::BatteryStatus::Stable){
            verifyDischargeWithBatterySuggestPowerStable(params.userSuggestStatus,
                                                            userSuggestPower, params.pcsCurrentSettingPower,
                                                            userSuggestSoc, params.batteryCurrentSoc);
            return;
        }

        // 电池推荐：提高功率
        if(batteryDischargeStatus == bau::BatteryStatus::SuggestUp){
            verifyDischargeWithBatterySuggestPowerUp(params.userSuggestStatus,
                                                        userSuggestPower, params.pcsCurrentSettingPower,
                                                        userSuggestSoc, params.batteryCurrentSoc);
            return;
        }
        return;
    }

    // 当前待机
    if(params.pcsCurrentStatus == "standby")
    {
        BOOST_ASSERT(params.pcsCurrentSettingPower == 0.0);

        // 用户推荐：充电
        if(params.userSuggestStatus == "charge"){
            // 提高功率
            double newPower = params.pcsCurrentSettingPower + 1.0;
            pcsControler_->setActivePower("charge", newPower);
            return;
        }

        // 用户推荐：放电
        if(params.userSuggestStatus == "discharge"){
            // 提高功率
            double newPower = params.pcsCurrentSettingPower + 1.0;
            pcsControler_->setActivePower("discharge", newPower);
            return;
        }

        // 用户推荐：待机
        if(params.userSuggestStatus == "standby"){
            // 保持待机
            return;
        }
        return;
    }
}

void StrategyXftg::saveNewPowerHistory(const double newPower)
{
    const string key{ "ems:strategy:xftg:output_power" };
    const long historyMaxSize{ 3600 * 10 };// 10h
    const string nowTimeStr = miscellaneous::getCurrentTimestamp();
    ostringstream oss;
    oss << fixed << setprecision(1) << newPower;//格式化保留1位小数
    // redis_->lpush(key, nowTimeStr + " => " + oss.str());
}

void StrategyXftg::verifyChargeWithBatterySuggestPowerDown(const double pcsCurrentSettingPower)
{
    BOOST_ASSERT(pcsCurrentSettingPower >= 0.0);

    // 降低功率
    double newPower = pcsCurrentSettingPower;
    if(newPower > 1.0) newPower -= 1.0;
    else newPower = 0.0;
    pcsControler_->setActivePower("charge", newPower);
}

void StrategyXftg::verifyChargeWithBatterySuggestPowerStable(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc)
{
    // 维度1：用户推荐方向

    // 维度2：用户推荐功率 与 PCS当前设置功率 之间的差值 => 与维度1组合起来，延伸出功率变化方向

    // 维度3：用户推荐充电深度SOC 与电池当前实际充电深度SOC 之间的差值  => 延伸出'是否待机'

    if(userSuggestStatus == "charge"
        && userSuggestPower >= pcsCurrentSettingPower
        && userSuggentSoc > batteryCurrentSoc){

        BOOST_ASSERT(pcsCurrentSettingPower >= 0.0);

        // 稳定功率
        double newPower = pcsCurrentSettingPower;
        pcsControler_->setActivePower("charge", newPower);
    }
    else{
        // 降低功率
        double newPower = pcsCurrentSettingPower;
        if(newPower > 1.0) newPower -= 1.0;
        else newPower = 0.0;
        pcsControler_->setActivePower("charge", newPower);
    }
}

void StrategyXftg::verifyChargeWithBatterySuggestPowerUp(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc)
{
    // 维度1：用户推荐方向

    // 维度2：用户推荐功率 与 PCS当前设置功率 之间的差值 => 与维度1组合起来，延伸出功率变化方向

    // 维度3：用户推荐充电深度SOC 与电池当前实际充电深度SOC 之间的差值  => 延伸出'是否待机'

    if(userSuggestStatus == "charge"){
        if(userSuggestPower > pcsCurrentSettingPower){
            if(userSuggentSoc > batteryCurrentSoc){
                // 提高功率
                double newPower = pcsCurrentSettingPower + 1.0;
                pcsControler_->setActivePower("charge", newPower);
            }
            else{
                // 降低功率
                double newPower = pcsCurrentSettingPower;
                if(newPower > 1.0) newPower -= 1.0;
                else newPower = 0.0;
                pcsControler_->setActivePower("charge", newPower);
            }
        }
        else if(userSuggestPower == pcsCurrentSettingPower){
            if(userSuggentSoc > batteryCurrentSoc){
                // 稳定功率
                double newPower = pcsCurrentSettingPower;
                pcsControler_->setActivePower("charge", newPower);
            }
            else{
                // 降低功率
                double newPower = pcsCurrentSettingPower;
                if(newPower > 1.0) newPower -= 1.0;
                else newPower = 0.0;
                pcsControler_->setActivePower("charge", newPower);
            }
        }
        else{
            // 降低功率
            double newPower = pcsCurrentSettingPower;
            if(newPower > 1.0) newPower -= 1.0;
            else newPower = 0.0;
            pcsControler_->setActivePower("charge", newPower);
        }
    }
    else{
        // 降低功率
        double newPower = pcsCurrentSettingPower;
        if(newPower > 1.0) newPower -= 1.0;
        else newPower = 0.0;
        pcsControler_->setActivePower("charge", newPower);
    }
}

void StrategyXftg::verifyDischargeWithBatterySuggestPowerDown(const double pcsCurrentSettingPower)
{
    // 降低功率
    double newPower = pcsCurrentSettingPower;
    if(newPower > 1.0) newPower -= 1.0;
    else newPower = 0.0;
    pcsControler_->setActivePower("discharge", newPower);
}

void StrategyXftg::verifyDischargeWithBatterySuggestPowerStable(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc)
{
    // 维度1：用户推荐方向

    // 维度2：用户推荐功率 与 PCS当前设置功率 之间的差值 => 与维度1组合起来，延伸出功率变化方向

    // 维度3：用户推荐充电深度SOC 与电池当前实际充电深度SOC 之间的差值  => 延伸出'是否待机'

    if(userSuggestStatus == "discharge"
        && userSuggestPower >= pcsCurrentSettingPower
        && userSuggentSoc > batteryCurrentSoc){
        // 稳定功率
        double newPower = pcsCurrentSettingPower;
        pcsControler_->setActivePower("discharge", newPower);
    }
    else{
        // 降低功率
        double newPower = pcsCurrentSettingPower;
        if(newPower > 1.0) newPower -= 1.0;
        else newPower = 0.0;
        pcsControler_->setActivePower("discharge", newPower);
    }
}

void StrategyXftg::verifyDischargeWithBatterySuggestPowerUp(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc)
{
    // 维度1：用户推荐方向

    // 维度2：用户推荐功率 与 PCS当前设置功率 之间的差值 => 与维度1组合起来，延伸出功率变化方向

    // 维度3：用户推荐充电深度SOC 与电池当前实际充电深度SOC 之间的差值  => 延伸出'是否待机'

    if(userSuggestStatus == "discharge"){
        if(userSuggestPower > pcsCurrentSettingPower){
            if(userSuggentSoc > batteryCurrentSoc){
                // 提高功率
                double newPower = pcsCurrentSettingPower + 1.0;
                pcsControler_->setActivePower("discharge", newPower);
            }
            else{
                // 降低功率
                double newPower = pcsCurrentSettingPower;
                if(newPower > 1.0) newPower -= 1.0;
                else newPower = 0.0;
                pcsControler_->setActivePower("discharge", newPower);
            }
        }
        else if(userSuggestPower == pcsCurrentSettingPower){
            if(userSuggentSoc > batteryCurrentSoc){
                // 稳定功率
                double newPower = pcsCurrentSettingPower;
                pcsControler_->setActivePower("discharge", newPower);
            }
            else{
                // 降低功率
                double newPower = pcsCurrentSettingPower;
                if(newPower > 1.0) newPower -= 1.0;
                else newPower = 0.0;
                pcsControler_->setActivePower("discharge", newPower);
            }
        }
        else{
            // 降低功率
            double newPower = pcsCurrentSettingPower;
            if(newPower > 1.0) newPower -= 1.0;
            else newPower = 0.0;
            pcsControler_->setActivePower("discharge", newPower);
        }
    }
    else{
        // 降低功率
        double newPower = pcsCurrentSettingPower;
        if(newPower > 1.0) newPower -= 1.0;
        else newPower = 0.0;
        pcsControler_->setActivePower("discharge", newPower);
    }
}

void StrategyXftg::setWeekPlanInfo(const std::map<string, WeekPlanInfo>& weekPlanInfoList)
{
    cout << "setWeekPlanInfo" << endl;
    weekPlanInfoMap_ = weekPlanInfoList;
}

void StrategyXftg::setDurationInfo(const pair<string, std::vector<DurationInfo>>& durationList)
{
    cout << "setDurationInfo" << endl;
    dayPlanDurationMap_.insert(durationList);
}

string StrategyXftg::getDayPlanName()
{
    const auto [weekPlanName, weekPlan] = getWeekPlan();
    return weekPlan.dayPlanDurationName;
}

std::tuple<DurationInfo, ProtectParams> StrategyXftg::getTarget()
{
    // 获取周计划
    const auto [weekPlanName, weekPlan] = getWeekPlan();

    // 根据周计划中的'日计划'名称，查找日计划参数
    const string dayPlanDurationName = weekPlan.dayPlanDurationName;
    const string dayPlanProtectName = weekPlan.dayPlanProtectName;

    // 根据当前系统时间，匹配'日计划'中的当前生效时段，并获取动作类型和执行功率
    const DurationInfo dayPlanDuration = getCurrentDurationInDayPlan(dayPlanDurationName);
    const ProtectParams protectParams = getCurrentProtectParamsInWeekPlan(dayPlanProtectName);
    return { dayPlanDuration, protectParams };
}

std::pair<string, WeekPlanInfo> StrategyXftg::getWeekPlan()
{
    // 获取'当前日期'(%Y-%m-%d)，用于匹配生效的'周计划'
    using Clock = std::chrono::system_clock;
    auto time_t_now = Clock::to_time_t(Clock::now());
    const tm* tmPtr = localtime(&time_t_now);
    stringstream ss;
    ss << std::put_time(tmPtr, "%F");
    const string currentDateString{ ss.str() };

    // 获取当前为星期几
    const string currentDayOfWeek = std::to_string(tmPtr->tm_wday);// [0, 6]

    // 获取有效期内且存在于周内白名单的'周计划'
    vector<std::pair<string, WeekPlanInfo>> validList;
    for(auto& item: weekPlanInfoMap_)
    {
        auto& weekPlan = item.second;
        if(currentDateString >= weekPlan.validDateBegin
            && currentDateString <= weekPlan.validDateEnd){

            // 实际上，这里是从列表中查找，当前是伪代码
            // 另外，绑定子系统的逻辑，也可以在此处完成匹配
            // 但是，匹配参数应该由外部传入，暂时没有传入参数。
            auto findResult = std::find_if(weekPlan.dayWhiteList.begin(), weekPlan.dayWhiteList.end(),
                [&](const int& ele){
                    return to_string(ele) == currentDayOfWeek;
                });
            if(findResult != weekPlan.dayWhiteList.end()){
                validList.push_back(item);
            }
        }
    }
    BOOST_ASSERT(!validList.empty());

    // 获取优先级最高的'周计划'
    auto maxPriorityItr = std::max_element(validList.begin(), validList.end(),
                                            [&](const std::pair<string, WeekPlanInfo>& lst, const std::pair<string, WeekPlanInfo>& rst){
        return lst.second.priority < rst.second.priority;
    });
    BOOST_ASSERT(maxPriorityItr != validList.end());

    // 至此，已经完成'周计划'的查找
    return *maxPriorityItr;
}

/*
    根据DayPlan名称，查找当前系统时间所在的时段计划
*/
DurationInfo StrategyXftg::getCurrentDurationInDayPlan(const string& name)
{
    // 获取'当前时间'(%H:%M)，用于匹配生效的'日计划'
    using Clock = std::chrono::system_clock;
    auto time_t_now = Clock::to_time_t(Clock::now());
    const tm* tmPtr = localtime(&time_t_now);
    stringstream ss;
    ss << std::put_time(tmPtr, "%H:%M");
    const string currentTimeString{ ss.str() };

    for(const auto& item : dayPlanDurationMap_[name]){

    }

    const auto & dayPlanDuration = dayPlanDurationMap_[name];
    auto findResult = std::find_if(dayPlanDuration.begin(), dayPlanDuration.end(),
                    [&](const DurationInfo& ele){
                        return ele.durationBegin <= currentTimeString
                                && ele.durationEnd > currentTimeString;
                    });
    BOOST_ASSERT(findResult != dayPlanDuration.end());
    return *findResult;
}

ProtectParams StrategyXftg::getCurrentProtectParamsInWeekPlan(const string& name)
{
    return protectPrarmsMap_[name];
}

// double StrategyXftg::getAdjustedPowerForSmoothOutput(const double srcPower, const double currentPower)
// {
//     const double diffPower = srcPower - currentPower;
//     if(diffPower == 0){
//         // log_.debug("计划功率与当前设置功率保持一致");
//         return srcPower;
//     }

//     const double largeStep = 5.0;
//     const double smallOStep = 1.0;
//     if(abs(diffPower) > 20){
//         // log_.debug("选择较大的功率调节步长");
//         if(diffPower > 0){
//             // log_.debug("叠加正向步长");
//             return currentPower + largeStep;
//         }
//         // log_.debug("叠加反向步长");
//         return currentPower + largeStep * (-1);
//     }

//     // log_.debug("选择较小的功率调节步长");
//     if(diffPower > 0)
//     {
//         // log_.debug("叠加正向步长");
//         return currentPower + smallOStep;
//     }
//     // log_.debug("叠加反向步长");
//     return currentPower + smallOStep * (-1);
// }
