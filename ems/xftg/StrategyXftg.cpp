#include "StrategyXftg.h"
#include <iostream>
#include <filesystem> // since c++17
#include <regex>
#include "boost/assert.hpp"
#include "DayPlanDuration.h"
#include "DayPlanProtect.h"
#include "WeekPlan.h"
#include "Setting.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/crc16.h"
#include "utils/datetime.h"
#include "utils/endian.h"
#include "ems/base/StationInfo.h"
#include "utils/ModbusRtu.h"

using namespace std;
using namespace std::chrono;
using namespace boost;
using namespace ems;
using namespace ems::xftg;

StrategyXftg::StrategyXftg(std::shared_ptr<zmq::socket_t> dataDealer, std::shared_ptr<zmq::socket_t> cmdDealer, std::shared_ptr<zmq::socket_t> setDealer)
    : log_(ems::Log4cppWrapper::getLogger(2))
    , dataDealer_(dataDealer)
    , cmdDealer_(cmdDealer)
    , setDealer_(setDealer)
{
    setDealer_->set(zmq::sockopt::rcvtimeo, 1000);
    
    loadAutoRun();
    loadWeekPlanInfo();
    loadDayPlanDurationInfo();
    loadDayPlanProtectInfo();
}

StrategyXftg::~StrategyXftg()
{
    if(loopThread_.joinable()){
        loopThread_.join();
    }
}

void StrategyXftg::start()
{
    loopThread_ = std::thread([&]{
    while(true){
        doWork();
    }});
}

void StrategyXftg::doWork()
{
try
{
    zmq::message_t srcIdentity;
    auto result = setDealer_->recv(srcIdentity);// 注意，这里主动向策略发消息的，只能是interface
    if(result.has_value()){

        // 接收用户接口控制，需要返回
        doCommand(srcIdentity);
        return;
    }

    // 请求站点更新数据
    StationInfo stationInfo = base::getStationInfo(dataDealer_);// 同步请求/响应

    // 基于最新数据进行控制
    control(stationInfo);// 同步请求/响应

    // 向上发布策略状态, 只有发送没有接收
    auto serializedBody = msgpackWrapper::pack(strategyInfo_);
    dataDealer_->send(zmq::message_t(string("PublishInfo")), zmq::send_flags::sndmore);
    dataDealer_->send(zmq::message_t(string("Strategy")), zmq::send_flags::sndmore);
    dataDealer_->send(zmq::message_t(string("0")), zmq::send_flags::sndmore);
    dataDealer_->send(zmq::message_t(serializedBody.data(), serializedBody.size()), zmq::send_flags::none);
    log_.debugStream() << "normal loop";
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
}
}

void StrategyXftg::control(const StationInfo& stationInfo)
{
try
{
    if(!strategyInfo_.autoRun)// 自动运行开关检查
        return;
    /////////////////////////////////////////////////////////////////

    const auto& bauMap = stationInfo.bauMap_;
    const auto& pcsMap = stationInfo.pcsMap_;
    if(bauMap.find(0) == bauMap.end() || pcsMap.find(0) == pcsMap.end()){
        return;
    }
    const auto& bauInfo = stationInfo.bauMap_.at(0);
    const auto& pcsInfo = stationInfo.pcsMap_.at(0);

    const auto bauParams =  getBauParams(bauInfo);
    const auto pcsParams =  getPcsParams(pcsInfo);
    const auto userParams =  getUserParams();

    // 执行策略计划，并获得控制命令
    auto frameResult = getRespondCmd(bauParams, pcsParams, userParams);
    if(frameResult.has_value()){
        // 请求
        vector<uint8_t> data = frameResult.value();
        cmdDealer_->send(zmq::message_t(string("PCS0Cmd")), zmq::send_flags::sndmore);// devId
        cmdDealer_->send(zmq::message_t(string("Strategy0Cmd")), zmq::send_flags::sndmore);// return id
        cmdDealer_->send(zmq::message_t(data.data(), data.size()), zmq::send_flags::none);

        zmq::message_t msg;
        (void)cmdDealer_->recv(msg);

        // 解析消息
        pair<bool, vector<uint8_t>> respondMsg;
        const bool unpackMsgResult = msgpackWrapper::unpack(msg.data(), msg.size(), respondMsg);
        BOOST_ASSERT(unpackMsgResult);
        const auto& [returnStatus, returnContent] = respondMsg;
        if(!returnStatus)
            throw std::runtime_error("modbus respond failed");
    }
}
catch(const std::exception& e)
{
    // 控制执行异常时，将整体策略状态切换为'手动';
    // 情况一：找不到有效的周策略
    strategyInfo_.autoRun = false;
    strategyInfo_.activeStrategy = "无";

    std::cerr << e.what() << '\n';
}
}

zmq::message_t StrategyXftg::createMsg(const uint16_t regAddress, const uint16_t regData)
{
    auto reqmsg = modbus::modbusRtuWriteFrame(0x01, 0x06, regAddress, regData);
    return { reqmsg.data(), reqmsg.size() };
}

void StrategyXftg::doCommand(zmq::message_t& srcIdentity)
{
    // type1 : 由 interface接收下发的，需要响应;
    // type2 : 来自PCS的控制响应帧，不需要响应;
    // type3 : 
    zmq::message_t msg;
    (void)setDealer_->recv(msg);
    const string msgString(static_cast<char*>(msg.data()), msg.size());

    if(msgString == "AutoRun"){
        // 修改自动运行标志 ?
        loadAutoRun();
    }
    else if(msgString == "WeekPlan"){
        // 重新加载削峰填谷-周计划 ?
        loadWeekPlanInfo();
    }
    else if(msgString == "DayPlanDuration"){
        // 重新加载削峰填谷-日计划 ?
        loadDayPlanDurationInfo();
    }
    else if(msgString == "DayPlanProtect"){
        // 重新加载削峰填谷-保护参数 ?
        loadDayPlanProtectInfo();
    }
    
    pair<bool, string> respondMsg{true, {}};
    auto serializedBody = msgpackWrapper::pack(respondMsg);
    setDealer_->send(srcIdentity, zmq::send_flags::sndmore);
    setDealer_->send(zmq::message_t(serializedBody.data(), serializedBody.size()), zmq::send_flags::none);
}


void StrategyXftg::loadAutoRun()
{
    auto flag = Setting::getRecord("0");
    if(flag.has_value()){
        const string autoRun = flag.value();
        if(autoRun == "true")
            strategyInfo_.autoRun = true;
        else
            strategyInfo_.autoRun = false;
    }
}

void StrategyXftg::loadWeekPlanInfo()
{
    // 从数据库表加载周计划
    weekPlanInfoMap_.clear();// 清空旧数据
    const auto records = WeekPlan::getAllRecords();
    if(!records.has_value())
        return;

    for(const auto& item : records.value()){

        WeekPlanInfo info;
        info.dayPlanDurationName = std::get<1>(item);
        info.dayPlanProtectName = std::get<2>(item);
        info.dayWhiteList = WeekPlan::convertDayofWeekListfromString(std::get<3>(item));
        info.validDateBegin = std::get<4>(item);
        info.validDateEnd = std::get<5>(item);
        info.priority = std::get<6>(item);
        info.bindSystem = std::get<7>(item);

        const string name = std::get<0>(item);
        weekPlanInfoMap_.emplace(name, info);
    }
}

void StrategyXftg::loadDayPlanDurationInfo()
{
    // 从数据库加载日计划
    const auto recordsMap = DayPlanDuration::getAllRecord();
    BOOST_ASSERT(recordsMap.has_value());

    for(const auto& records : recordsMap.value()){

        auto& durationInfoList = dayPlanDurationMap_[records.first];

        const vector<DayPlanDuration::Record>& durations = records.second;
        for(auto& [durationName, 
            durationBegin, durationEnd, controlType, targetSoc, targetPower] : durations){

            DurationInfo info;
            info.durationName = durationName;
            info.durationBegin = durationBegin;
            info.durationEnd = durationEnd;
            info.controlType = controlType;
            {
                std::regex pattern(R"((\d+)%)");// 匹配类似100%中的数值
                std::smatch matches;
                const bool searchResult = std::regex_search(targetSoc, matches, pattern);
                BOOST_ASSERT(searchResult);
                info.targetSoc = std::stoi(matches[1].str());
            }
            {
                std::regex pattern(R"((\d+)kW)");// 匹配类似100kW中的数值
                std::smatch matches;
                const bool searchResult = std::regex_search(targetPower, matches, pattern);
                BOOST_ASSERT(searchResult);
                info.targetPower = std::stod(matches[1].str());
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

vector<uint8_t> StrategyXftg::getFramePowerOff()
{
    vector<uint8_t> requestMessage(8);
    {
        requestMessage[0] = 0x01;
        requestMessage[1] = 0x06;
        {
            uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestMessage[2]);
            *registerAddrPtr = 0x04CC;//开关机方式
            endian::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
        }
        {
            uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestMessage[4]);
            *registerDataPtr = 0xAAAA;// 关机
            endian::reverseByteArray(registerDataPtr, sizeof(uint16_t));
        }
        {
            uint16_t crc16Modbus = modbus::crc16_manual(requestMessage.data(), 6);
            std::memcpy(&requestMessage[6], &crc16Modbus, sizeof(uint16_t));
        }
    }
    return requestMessage;
}

vector<uint8_t> StrategyXftg::getFramePowerOn()
{
    vector<uint8_t> requestMessage(8);
    {
        requestMessage[0] = 0x01;
        requestMessage[1] = 0x06;
        {
            uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestMessage[2]);
            *registerAddrPtr = 0x04CC;//开关机方式
            endian::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
        }
        {
            uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestMessage[4]);
            *registerDataPtr = 0x5555;// 开机
            endian::reverseByteArray(registerDataPtr, sizeof(uint16_t));
        }
        {
            uint16_t crc16Modbus = modbus::crc16_manual(requestMessage.data(), 6);
            std::memcpy(&requestMessage[6], &crc16Modbus, sizeof(uint16_t));
        }
    }
    return requestMessage;
}

vector<uint8_t> StrategyXftg::getFrameChangeActivePower(const string& status, const double power)
{
    BOOST_ASSERT(power >= 0 && power <= 50.0);
    BOOST_ASSERT(status == "charge" || status == "discharge");

    uint16_t activePower = static_cast<uint16_t>(power * 0.1);//单位:0.1kW
    if(status == "charge") activePower *= (-1);

    vector<uint8_t> requestMessage(8);
    {
        requestMessage[0] = 0x01;
        requestMessage[1] = 0x06;
        {
            uint16_t* registerAddrPtr = reinterpret_cast<uint16_t*>(&requestMessage[2]);
            *registerAddrPtr = 0x04C4;
            endian::reverseByteArray(registerAddrPtr, sizeof(uint16_t));
        }
        {
            uint16_t* registerDataPtr = reinterpret_cast<uint16_t*>(&requestMessage[4]);
            *registerDataPtr = activePower;
            endian::reverseByteArray(registerDataPtr, sizeof(uint16_t));
        }
        {
            uint16_t crc16Modbus = modbus::crc16_manual(requestMessage.data(), 6);
            std::memcpy(&requestMessage[6], &crc16Modbus, sizeof(uint16_t));
        }
    }
    return requestMessage;
}

std::optional<vector<uint8_t>> StrategyXftg::getActionWhenCharge(const BauParams& bauParams, const PcsParams& pcsParams, const UserParams& userParams)
{
    // 禁止充电
    if(!bauParams.bauAllowCharge){
        // 降低功率
        double newPower = pcsParams.pcsCurrentSettingPower;
        if(newPower > 1.0) newPower -= 1.0;
        else newPower = 0.0;
        return getFrameChangeActivePower("charge", newPower);
    }

    // 校验充电保护参数(功率);
    double userSuggestPower = userParams.userSuggestPower;
    if(userSuggestPower > userParams.allowChargePowerMax){
        userSuggestPower = userParams.allowChargePowerMax;
    }

    // 校验充电保护参数(SOC);
    int userSuggestSoc = userParams.userSuggestSoc;
    if(userSuggestSoc > userParams.allowSocMax){
        userSuggestSoc = userParams.allowSocMax;
    }

    // 电池推荐：降功率 => 降功率：结束
    if(bauParams.batteryChargeStatus == bau::BatteryStatus::SuggestDown){
        return verifyChargeWithBatterySuggestPowerDown(pcsParams.pcsCurrentSettingPower);
    }

    // 电池推荐：稳定输出
    if(bauParams.batteryChargeStatus == bau::BatteryStatus::Stable){
        
        return verifyChargeWithBatterySuggestPowerStable(userParams.userSuggestStatus,
                                                    userSuggestPower, pcsParams.pcsCurrentSettingPower,
                                                    userSuggestSoc, bauParams.batteryCurrentSoc);
    }

    // 电池推荐：提高功率
    if(bauParams.batteryChargeStatus == bau::BatteryStatus::SuggestUp){
        return verifyChargeWithBatterySuggestPowerUp(userParams.userSuggestStatus,
                                                userSuggestPower, pcsParams.pcsCurrentSettingPower,
                                                userSuggestSoc, bauParams.batteryCurrentSoc);
    }
    return {};
}

std::optional<vector<uint8_t>> StrategyXftg::getActionWhenDischarge(const BauParams& bauParams, const PcsParams& pcsParams, const UserParams& userParams)
{
    // 禁止放电
    if(!bauParams.bauAllowDischarge){
        // 降低功率
        double newPower = pcsParams.pcsCurrentSettingPower;
        if(newPower > 1.0) newPower -= 1.0;
        else newPower = 0.0;
        return getFrameChangeActivePower("discharge", newPower);
    }

    // 校验放电保护参数(功率);
    double userSuggestPower = userParams.userSuggestPower;
    if(userSuggestPower > userParams.allowDischargePowerMax){
        userSuggestPower = userParams.allowDischargePowerMax;
    }

    // 校验放电保护参数(SOC);
    int userSuggestSoc = userParams.userSuggestSoc;
    if(userSuggestSoc < userParams.allowSocMin){
        userSuggestSoc = userParams.allowSocMin;
    }

    // 电池推荐：降功率 => 降功率：结束
    if(bauParams.batteryDischargeStatus == bau::BatteryStatus::SuggestDown){
        return verifyDischargeWithBatterySuggestPowerDown(pcsParams.pcsCurrentSettingPower);
    }

    // 电池推荐：稳定输出
    if(bauParams.batteryDischargeStatus == bau::BatteryStatus::Stable){
        return verifyDischargeWithBatterySuggestPowerStable(userParams.userSuggestStatus,
                                                        userSuggestPower, pcsParams.pcsCurrentSettingPower,
                                                        userSuggestSoc, bauParams.batteryCurrentSoc);
    }

    // 电池推荐：提高功率
    if(bauParams.batteryDischargeStatus == bau::BatteryStatus::SuggestUp){
        return verifyDischargeWithBatterySuggestPowerUp(userParams.userSuggestStatus,
                                                    userSuggestPower, pcsParams.pcsCurrentSettingPower,
                                                    userSuggestSoc, bauParams.batteryCurrentSoc);
    }
    return {};
}

std::optional<vector<uint8_t>> StrategyXftg::getActionWhenStandby(const BauParams& bauParams, const PcsParams& pcsParams, const UserParams& userParams)
{
    // 用户推荐：充电
    if(userParams.userSuggestStatus == "charge"){
        // 提高功率
        double newPower = pcsParams.pcsCurrentSettingPower + 1.0;
        return getFrameChangeActivePower("charge", newPower);
    }

    // 用户推荐：放电
    if(userParams.userSuggestStatus == "discharge"){
        // 提高功率
        double newPower = pcsParams.pcsCurrentSettingPower + 1.0;
        return getFrameChangeActivePower("discharge", newPower);
    }

    // 用户推荐：待机
    if(userParams.userSuggestStatus == "standby"){
        // 保持待机
        return {};
    }
    return {};
}

UserParams StrategyXftg::getUserParams()
{
    // 提取策略所依赖的用户计划参数
    const auto& [durationInfo, protectParams] = getPlan();
    const string userSuggestStatus = durationInfo.controlType;
    const double userSuggestPower = durationInfo.targetPower;
    const int userSuggestSoc = durationInfo.targetSoc;
    const double allowChargePowerMax = protectParams.chargePowerMax;
    const double allowDischargePowerMax = protectParams.dischargePowerMax;
    const int allowSocMax = protectParams.socMax;
    const int allowSocMin = protectParams.socMin;

    const UserParams params{
        userSuggestStatus, userSuggestPower, userSuggestSoc,
        allowChargePowerMax, allowDischargePowerMax, allowSocMax, allowSocMin
    };
    return params;
}

PcsParams StrategyXftg::getPcsParams(const pcs::PcsInfo& pcsInfo)
{
    const double pcsCurrentSettingPower = pcsInfo.frame_0474_04D0_summary.activePowerSetting;
    const double pcsCurrentOutputPower = pcsInfo.frame_0474_04D0_summary.activePowerSetting;
    const pcs::RunStatus pcsCurrentStatus = pcsInfo.runStatus;

    const PcsParams params{
        pcsCurrentSettingPower, pcsCurrentOutputPower, pcsCurrentStatus
    };
    return params;
}

BauParams StrategyXftg::getBauParams(const bau::BauInfo& bauInfo)
{
    /* 实时状态： 可运行、可充电、可放电 */
    const bool bauAllowRunning = bauInfo.allowRunning;
    const bool bauAllowCharge = bauInfo.allowCharge;
    const bool bauAllowDischarge = bauInfo.allowDischarge;

    /* 实时电压、电流、SOC */
    const double batteryCurrentVolt = bauInfo.bauStatusSummary.volt;
    const double batteryCurrentCur = bauInfo.bauStatusSummary.cur;
    const int batteryCurrentSoc = bauInfo.bauStatusSummary.soc;

    /* 根据电压/电流 的推荐值与实际值 作比较，得到电流的推荐动作：稳定功率、提升功率或降低功率 */
    const double batterySuggestChargeVolt = bauInfo.bauStatusSummary.pcsRequestChargeVolt;
    const double batterySuggestChargeCur = bauInfo.bauStatusSummary.pcsRequestChargeCur;
    const double batterySuggestDischargeVolt = bauInfo.bauStatusSummary.pcsRequestDischargeVolt;
    const double batterySuggestDischargeCur = bauInfo.bauStatusSummary.pcsRequestDischargeCur;
    const auto batteryChargeStatus = bauStatus_.getChargeStatus(batteryCurrentVolt, batteryCurrentCur,
                                                                batterySuggestChargeVolt, batterySuggestChargeCur);
    const auto batteryDischargeStatus = bauStatus_.getDischargeStatus(batteryCurrentVolt, batteryCurrentCur,
                                                                            batterySuggestDischargeVolt, batterySuggestDischargeCur);
    const BauParams params{
        bauAllowRunning, bauAllowCharge, bauAllowDischarge,
        batteryCurrentVolt, batteryCurrentCur, batteryCurrentSoc,
        batteryChargeStatus, batteryDischargeStatus
    };
    return params;
}

std::optional<vector<uint8_t>> StrategyXftg::getRespondCmd(const BauParams& bauParams, const PcsParams& pcsParams, const UserParams& userParams)
{
    // 告警、故障位检查
    if(!bauParams.bauAllowRunning){
        if(pcsParams.pcsCurrentStatus.powerTotal)// 告警、故障产生，需要关机
            return getFramePowerOff();
        return {};// 已经关机了，直接退出。
    }
    if(!pcsParams.pcsCurrentStatus.powerTotal)
        return getFramePowerOn();// 本轮先开机，下轮再执行正常工作
    /////////////////////////////////////////////////////////////////

    // 当前充电
    if(pcsParams.pcsCurrentStatus.gridOnCharge)
        return getActionWhenCharge(bauParams, pcsParams, userParams);
    // 当前放电
    if(pcsParams.pcsCurrentStatus.gridOnDischarge)
        return getActionWhenDischarge(bauParams, pcsParams, userParams);
    // 当前待机
    if(pcsParams.pcsCurrentStatus.standby)
        return getActionWhenStandby(bauParams, pcsParams, userParams);
    return {};
}

vector<uint8_t> StrategyXftg::verifyChargeWithBatterySuggestPowerDown(const double pcsCurrentSettingPower)
{
    BOOST_ASSERT(pcsCurrentSettingPower >= 0.0);

    // 降低功率
    double newPower = pcsCurrentSettingPower;
    if(newPower > 1.0) newPower -= 1.0;
    else newPower = 0.0;
    return getFrameChangeActivePower("charge", newPower);
}

vector<uint8_t> StrategyXftg::verifyChargeWithBatterySuggestPowerStable(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc)
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
        return getFrameChangeActivePower("charge", newPower);
    }
    else{
        // 降低功率
        double newPower = pcsCurrentSettingPower;
        if(newPower > 1.0) newPower -= 1.0;
        else newPower = 0.0;
        return getFrameChangeActivePower("charge", newPower);
    }
    return {};
}

vector<uint8_t> StrategyXftg::verifyChargeWithBatterySuggestPowerUp(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc)
{
    // 维度1：用户推荐方向

    // 维度2：用户推荐功率 与 PCS当前设置功率 之间的差值 => 与维度1组合起来，延伸出功率变化方向

    // 维度3：用户推荐充电深度SOC 与电池当前实际充电深度SOC 之间的差值  => 延伸出'是否待机'

    if(userSuggestStatus == "charge"){
        if(userSuggestPower > pcsCurrentSettingPower){
            if(userSuggentSoc > batteryCurrentSoc){
                // 提高功率
                double newPower = pcsCurrentSettingPower + 1.0;
                return getFrameChangeActivePower("charge", newPower);
            }
            else{
                // 降低功率
                double newPower = pcsCurrentSettingPower;
                if(newPower > 1.0) newPower -= 1.0;
                else newPower = 0.0;
                return getFrameChangeActivePower("charge", newPower);
            }
        }
        else if(userSuggestPower == pcsCurrentSettingPower){
            if(userSuggentSoc > batteryCurrentSoc){
                // 稳定功率
                double newPower = pcsCurrentSettingPower;
                return getFrameChangeActivePower("charge", newPower);
            }
            else{
                // 降低功率
                double newPower = pcsCurrentSettingPower;
                if(newPower > 1.0) newPower -= 1.0;
                else newPower = 0.0;
                return getFrameChangeActivePower("charge", newPower);
            }
        }
        else{
            // 降低功率
            double newPower = pcsCurrentSettingPower;
            if(newPower > 1.0) newPower -= 1.0;
            else newPower = 0.0;
            return getFrameChangeActivePower("charge", newPower);
        }
    }
    else{
        // 降低功率
        double newPower = pcsCurrentSettingPower;
        if(newPower > 1.0) newPower -= 1.0;
        else newPower = 0.0;
        return getFrameChangeActivePower("charge", newPower);
    }
    return {};
}

vector<uint8_t> StrategyXftg::verifyDischargeWithBatterySuggestPowerDown(const double pcsCurrentSettingPower)
{
    // 降低功率
    double newPower = pcsCurrentSettingPower;
    if(newPower > 1.0) newPower -= 1.0;
    else newPower = 0.0;
    return getFrameChangeActivePower("discharge", newPower);
}

vector<uint8_t> StrategyXftg::verifyDischargeWithBatterySuggestPowerStable(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc)
{
    // 维度1：用户推荐方向

    // 维度2：用户推荐功率 与 PCS当前设置功率 之间的差值 => 与维度1组合起来，延伸出功率变化方向

    // 维度3：用户推荐充电深度SOC 与电池当前实际充电深度SOC 之间的差值  => 延伸出'是否待机'

    if(userSuggestStatus == "discharge"
        && userSuggestPower >= pcsCurrentSettingPower
        && userSuggentSoc > batteryCurrentSoc){
        // 稳定功率
        double newPower = pcsCurrentSettingPower;
        return getFrameChangeActivePower("discharge", newPower);
    }
    else{
        // 降低功率
        double newPower = pcsCurrentSettingPower;
        if(newPower > 1.0) newPower -= 1.0;
        else newPower = 0.0;
        return getFrameChangeActivePower("discharge", newPower);
    }
    return {};
}

vector<uint8_t> StrategyXftg::verifyDischargeWithBatterySuggestPowerUp(const string& userSuggestStatus, const double userSuggestPower, const double pcsCurrentSettingPower, const int userSuggentSoc, const int batteryCurrentSoc)
{
    // 维度1：用户推荐方向

    // 维度2：用户推荐功率 与 PCS当前设置功率 之间的差值 => 与维度1组合起来，延伸出功率变化方向

    // 维度3：用户推荐充电深度SOC 与电池当前实际充电深度SOC 之间的差值  => 延伸出'是否待机'

    if(userSuggestStatus == "discharge"){
        if(userSuggestPower > pcsCurrentSettingPower){
            if(userSuggentSoc > batteryCurrentSoc){
                // 提高功率
                double newPower = pcsCurrentSettingPower + 1.0;
                return getFrameChangeActivePower("discharge", newPower);
            }
            else{
                // 降低功率
                double newPower = pcsCurrentSettingPower;
                if(newPower > 1.0) newPower -= 1.0;
                else newPower = 0.0;
                return getFrameChangeActivePower("discharge", newPower);
            }
        }
        else if(userSuggestPower == pcsCurrentSettingPower){
            if(userSuggentSoc > batteryCurrentSoc){
                // 稳定功率
                double newPower = pcsCurrentSettingPower;
                return getFrameChangeActivePower("discharge", newPower);
            }
            else{
                // 降低功率
                double newPower = pcsCurrentSettingPower;
                if(newPower > 1.0) newPower -= 1.0;
                else newPower = 0.0;
                return getFrameChangeActivePower("discharge", newPower);
            }
        }
        else{
            // 降低功率
            double newPower = pcsCurrentSettingPower;
            if(newPower > 1.0) newPower -= 1.0;
            else newPower = 0.0;
            return getFrameChangeActivePower("discharge", newPower);
        }
    }
    else{
        // 降低功率
        double newPower = pcsCurrentSettingPower;
        if(newPower > 1.0) newPower -= 1.0;
        else newPower = 0.0;
        return getFrameChangeActivePower("discharge", newPower);
    }
    return {};
}

tuple<DurationInfo, ProtectParams> StrategyXftg::getPlan()
{
    // 获取周计划
    const auto [weekPlanName, weekPlan] = getWeekPlan();
    strategyInfo_.activeStrategy = weekPlanName;

    // 根据周计划中的'日计划'名称，查找日计划参数
    const string dayPlanDurationName = weekPlan.dayPlanDurationName;
    const string dayPlanProtectName = weekPlan.dayPlanProtectName;

    // 根据当前系统时间，匹配'日计划'中的当前生效时段，并获取动作类型和执行功率
    const DurationInfo dayPlanDuration = getCurrentDurationInDayPlan(dayPlanDurationName);
    const ProtectParams protectParams = getCurrentProtectParamsInWeekPlan(dayPlanProtectName);
    return make_tuple(dayPlanDuration, protectParams);
}

pair<string, WeekPlanInfo> StrategyXftg::getWeekPlan()
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
    if(validList.empty())
        throw std::runtime_error("未找到有效的周计划");

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

    const auto & dayPlanDuration = dayPlanDurationMap_[name];
    auto findResult = std::find_if(dayPlanDuration.begin(), dayPlanDuration.end(),
                    [&](const DurationInfo& ele){
                        return ele.durationBegin <= currentTimeString
                                && ele.durationEnd > currentTimeString;
                    });
    if(findResult == dayPlanDuration.end())
        throw std::runtime_error("未找到有效的执行时段");
    return *findResult;
}

ProtectParams StrategyXftg::getCurrentProtectParamsInWeekPlan(const string& name)
{
    return protectPrarmsMap_[name];
}
