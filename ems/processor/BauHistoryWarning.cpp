#include "BauHistoryWarning.h"
#include "boost/assert.hpp"
#include <optional>
#include "ems/base/HistoryWarning.h"
#include "utils/Miscellaneous.h"
#include "utils/datetime.h"

using namespace std;
using namespace ems::processed;

BauHistoryWarning::BauHistoryWarning()
{
}

void BauHistoryWarning::handleBranch(const int branchIndex, const bau::BauInfo& beforeInfo, const bau::BauInfo& currentInfo)
{
    const string bauName{ "BAU" + to_string(branchIndex) + '#' };
    handleBau(bauName, beforeInfo.bauStatusSummary, currentInfo.bauStatusSummary);

    for(const auto& currentItem : currentInfo.bcuList){
        const int currentIndex = currentItem.first;
        const string bcuName{ bauName + "BCU" + to_string(currentIndex) + '#' };

        const auto& beforeBcuList = beforeInfo.bcuList;
        auto result = beforeBcuList.find(currentIndex);
        if(result != beforeBcuList.end()){
            handleBcu(bcuName, beforeBcuList.at(currentIndex).base, currentItem.second.base);
        }
        else{
            handleNewBcu(bcuName, currentItem.second.base);
        }
    }
}

void BauHistoryWarning::handleBau(const string& devName, const bau::BauStatusSummary& before, const bau::BauStatusSummary& current)
{    
    handleBauProtectStatus(devName, "1", before.protectStatusL1, current.protectStatusL1);
    handleBauProtectStatus(devName, "2", before.protectStatusL2, current.protectStatusL2);
    handleBauProtectStatus(devName, "3", before.protectStatusL3, current.protectStatusL3);
    handleBauFaultStatus(devName, "3", before.faultStatus, current.faultStatus);
}

void BauHistoryWarning::handleBauProtectStatus(const string& devName, const string& level, const uint32_t before, const uint32_t current)
{
    const bitset<25> beforeBits(before);
    const bitset<25> currentBits(current);
    const string createTime = datetime::getCurrentTimestamp();
    
    save(devName, "单体高压", beforeBits[0], currentBits[0], level, createTime);
    save(devName, "单体低压", beforeBits[1], currentBits[1], level, createTime);
    save(devName, "总体高压", beforeBits[2], currentBits[2], level, createTime);
    save(devName, "总体低压", beforeBits[3], currentBits[3], level, createTime);
    save(devName, "充电过流", beforeBits[4], currentBits[4], level, createTime);
    save(devName, "放电过流", beforeBits[5], currentBits[5], level, createTime);
    save(devName, "充电高温", beforeBits[6], currentBits[6], level, createTime);
    save(devName, "放电高温", beforeBits[7], currentBits[7], level, createTime);
    save(devName, "充电低温", beforeBits[8], currentBits[8], level, createTime);
    save(devName, "放电低温", beforeBits[9], currentBits[9], level, createTime);
    save(devName, "环境高温", beforeBits[10], currentBits[10], level, createTime);
    save(devName, "环境低温", beforeBits[11], currentBits[11], level, createTime);
    save(devName, "充电继电器高温", beforeBits[12], currentBits[12], level, createTime);
    save(devName, "放电继电器高温", beforeBits[13], currentBits[13], level, createTime);
    save(devName, "负极继电器高温", beforeBits[14], currentBits[14], level, createTime);
    save(devName, "SOC高", beforeBits[15], currentBits[15], level, createTime);
    save(devName, "SOC低", beforeBits[16], currentBits[16], level, createTime);
    save(devName, "正极绝缘漏电", beforeBits[17], currentBits[17], level, createTime);
    save(devName, "负极绝缘漏电", beforeBits[18], currentBits[18], level, createTime);
    save(devName, "充电压差", beforeBits[19], currentBits[19], level, createTime);
    save(devName, "放电压差", beforeBits[20], currentBits[20], level, createTime);
    save(devName, "充电温差", beforeBits[21], currentBits[21], level, createTime);
    save(devName, "放电温差", beforeBits[22], currentBits[22], level, createTime);
    save(devName, "端子高温", beforeBits[23], currentBits[23], level, createTime);
    save(devName, "簇间压差", beforeBits[24], currentBits[24], level, createTime);
}

void BauHistoryWarning::handleNewBcu(const string& devName, const bau::BcuStatusSummary& current)
{
    const string createTime = datetime::getCurrentTimestamp();
    
    handleBcuProtectStatus(devName, "1", 0, current.protectStatusL1);
    handleBcuProtectStatus(devName, "2",0, current.protectStatusL2);
    handleBcuProtectStatus(devName, "3", 0, current.protectStatusL3);
    handleBcuFaultStatus(devName, "3", 0, current.faultStatus);
}

void BauHistoryWarning::handleBcu(const string& devName, const bau::BcuStatusSummary& before, const bau::BcuStatusSummary& current)
{
    const string createTime = datetime::getCurrentTimestamp();
    
    handleBcuProtectStatus(devName, "1", before.protectStatusL1, current.protectStatusL1);
    handleBcuProtectStatus(devName, "2", before.protectStatusL2, current.protectStatusL2);
    handleBcuProtectStatus(devName, "3", before.protectStatusL3, current.protectStatusL3);
    handleBcuFaultStatus(devName, "3", before.faultStatus, current.faultStatus);
}

void BauHistoryWarning::handleBcuProtectStatus(const string& devName, const string& level, const uint32_t before, const uint32_t current)
{
    const bitset<27> beforeBits(before);
    const bitset<27> currentBits(current);
    const string createTime = datetime::getCurrentTimestamp();

    save(devName, "单体高压", beforeBits[0], currentBits[0], level, createTime);
    save(devName, "单体低压", beforeBits[1], currentBits[1], level, createTime);
    save(devName, "总体高压", beforeBits[2], currentBits[2], level, createTime);
    save(devName, "总体低压", beforeBits[3], currentBits[3], level, createTime);
    save(devName, "充电过流", beforeBits[4], currentBits[4], level, createTime);
    save(devName, "放电过流", beforeBits[5], currentBits[5], level, createTime);
    save(devName, "充电高温", beforeBits[6], currentBits[6], level, createTime);
    save(devName, "放电高温", beforeBits[7], currentBits[7], level, createTime);
    save(devName, "充电低温", beforeBits[8], currentBits[8], level, createTime);
    save(devName, "放电低温", beforeBits[9], currentBits[9], level, createTime);
    save(devName, "环境高温", beforeBits[10], currentBits[10], level, createTime);
    save(devName, "环境低温", beforeBits[11], currentBits[11], level, createTime);
    save(devName, "充电继电器高温", beforeBits[12], currentBits[12], level, createTime);
    save(devName, "放电继电器高温", beforeBits[13], currentBits[13], level, createTime);
    save(devName, "负极继电器高温", beforeBits[14], currentBits[14], level, createTime);
    save(devName, "SOC高", beforeBits[15], currentBits[15], level, createTime);
    save(devName, "SOC低", beforeBits[16], currentBits[16], level, createTime);
    save(devName, "正极绝缘漏电", beforeBits[17], currentBits[17], level, createTime);
    save(devName, "负极绝缘漏电", beforeBits[18], currentBits[18], level, createTime);
    save(devName, "充电压差", beforeBits[19], currentBits[19], level, createTime);
    save(devName, "放电压差", beforeBits[20], currentBits[20], level, createTime);
    save(devName, "充电温差", beforeBits[21], currentBits[21], level, createTime);
    save(devName, "放电温差", beforeBits[22], currentBits[22], level, createTime);
    save(devName, "电芯温升", beforeBits[23], currentBits[23], level, createTime);
    save(devName, "电芯采样", beforeBits[24], currentBits[24], level, createTime);
    save(devName, "NTC采样异常", beforeBits[25], currentBits[25], level, createTime);
    save(devName, "端子高温", beforeBits[26], currentBits[26], level, createTime);
}

void BauHistoryWarning::handleBauFaultStatus(const string& devName, const string& level, const uint32_t before, const uint32_t current)
{
    const bitset<3> beforeBits(before);
    const bitset<3> currentBits(current);
    const string createTime = datetime::getCurrentTimestamp();

    save(devName, "CAN总线异常", beforeBits[0], currentBits[0], level, createTime);
    save(devName, "RS485异常", beforeBits[1], currentBits[1], level, createTime);
    save(devName, "BCU版本异常", beforeBits[2], currentBits[2], level, createTime);
}

void BauHistoryWarning::handleBcuFaultStatus(const string& devName, const string& level, const uint32_t before, const uint32_t current)
{
    const bitset<18> beforeBits(before);
    const bitset<18> currentBits(current);
    const string createTime = datetime::getCurrentTimestamp();

    save(devName, "充电继电器粘连", beforeBits[0], currentBits[0], level, createTime);
    save(devName, "充电继电器失效", beforeBits[1], currentBits[1], level, createTime);
    save(devName, "放电继电器粘连", beforeBits[2], currentBits[2], level, createTime);
    save(devName, "放电继电器失效", beforeBits[3], currentBits[3], level, createTime);
    save(devName, "预充继电器粘连", beforeBits[4], currentBits[4], level, createTime);
    save(devName, "预充继电器失效", beforeBits[5], currentBits[5], level, createTime);
    save(devName, "负极继电器粘连", beforeBits[6], currentBits[6], level, createTime);
    save(devName, "负极继电器失效", beforeBits[7], currentBits[7], level, createTime);
    save(devName, "加热膜继电器粘连", beforeBits[8], currentBits[8], level, createTime);
    save(devName, "加热膜继电器失效", beforeBits[9], currentBits[9], level, createTime);
    save(devName, "12V异常", beforeBits[10], currentBits[10], level, createTime);
    save(devName, "电芯故障", beforeBits[11], currentBits[11], level, createTime);
    save(devName, "预充故障", beforeBits[12], currentBits[12], level, createTime);
    save(devName, "加热膜故障", beforeBits[13], currentBits[13], level, createTime);
    save(devName, "绝缘板通信故障", beforeBits[14], currentBits[14], level, createTime);
    save(devName, "采样板通信故障", beforeBits[15], currentBits[15], level, createTime);
    save(devName, "电流分流器故障", beforeBits[16], currentBits[16], level, createTime);
    save(devName, "NTC故障", beforeBits[17], currentBits[17], level, createTime);
}

void BauHistoryWarning::save(const string& devName, const string& content, const bool before, const bool current,
        const string& level, const string& createTime)
{
    if(before == current) return;
    const string status = before ? "disable" : "enable";
    const string processed("no");
    base::HistoryWarning::insertRecord(content, level, devName, createTime, status, processed);
}
