#pragma once
#include "msgpack.hpp"
// #include "ems/base/BauWarning.h"

namespace ems
{
namespace bau
{
using namespace std;

struct CellvoltSummary
{
    using Volt = pair<uint16_t, double>;
    vector<Volt> cellvoltList;// 电芯电压

    /* 统计数据 */
    Volt cellvoltMax;// 最大电压
    Volt cellvoltMin;// 最小电压
    uint16_t voltDiff;// 压差

    MSGPACK_DEFINE(cellvoltList, cellvoltMax, cellvoltMin, voltDiff)
};

struct CelltemSummary
{
    using Tem = pair<uint16_t, double>;
    vector<Tem> cellTemList;// 电芯温度
    vector<Tem> terminalTemList;// 端子温度

    /* 统计数据 */
    Tem celltemMax;// 最大温度
    Tem celltemMin;// 最小温度
    uint16_t temDiff;// 温差

    MSGPACK_DEFINE(cellTemList, terminalTemList, celltemMax, celltemMin, temDiff)
};

struct BmuInfo
{
    /**
     * 基础信息:
    */
    CellvoltSummary cellvoltInfo;
    CelltemSummary celltemInfo;

    /* 统计信息 */

    MSGPACK_DEFINE(cellvoltInfo, celltemInfo);
};

//////////////////////////////////////////////////////////////////////////////////

struct BcuWarningStatus
{
    int cellVoltHigh;//单体高压
    int cellVoltLow;//单体低压
    int totalVoltHigh;//总体高压
    int totalVoltLow;//总体低压
    int chargeOverCur;//充电过流
    int dischargeOverCur;//放电过流
    int chargeTemHigh;//充电高温
    int dischargeTemHigh;//放电高温
    int chargeTemLow;//充电低温
    int dischargeTemLow;//放电低温
    int envTemHigh;//环境高温
    int envTemLow;//环境低温
    int chargeRelayTemHigh;//充电继电器高温
    int dischargeRelayTemHigh;//放电继电器高温
    int negativeRelayTemHigh;//负极继电器高温
    int socHigh;//SOC高
    int socLow;//SOC低
    int positiveInsulationLeakage;//正极绝缘漏电
    int negativeInsulationLeakage;//负极绝缘漏电
    int chargeVoltDiff;//充电压差
    int dischargeVoltDiff;//放电压差
    int chargeTemDiff;//充电温差
    int dischargeTemDiff;//放电温差

    int cellTemIncrease;//电芯温升
    int cellSampling;//电芯采样
    int ntcSamplingErr;//NTC采样异常
    int terminalTemHigh;//端子高温

    MSGPACK_DEFINE(cellVoltHigh, cellVoltLow, totalVoltHigh, totalVoltLow, chargeOverCur, dischargeOverCur,
                    chargeTemHigh, dischargeTemHigh, chargeTemLow, dischargeTemLow, envTemHigh, envTemLow,
                    chargeRelayTemHigh, dischargeRelayTemHigh, negativeRelayTemHigh, socHigh, socLow,
                    positiveInsulationLeakage, negativeInsulationLeakage, chargeVoltDiff, dischargeVoltDiff,
                    chargeTemDiff, dischargeTemDiff, cellTemIncrease, cellSampling, ntcSamplingErr, terminalTemHigh);
};

struct BcuFaultStatus
{
    bool chargeRelayCombine;//充电继电器粘连
    bool chargeRelayDisabled;//充电继电器失效
    bool dischargeRelayCombine;//放电继电器粘连
    bool dischargeRelayDisabled;//放电继电器失效
    bool prechargeRelayCombine;//预充继电器粘连
    bool prechargeRelayDisabled;//预充继电器失效
    bool negativeRelayCombine;//负极继电器粘连
    bool negativeRelayDisabled;//负极继电器失效
    bool heatingFilmRelayCombine;//加热膜继电器粘连
    bool heatingFileRelayDisabled;//加热膜继电器失效
    bool _12VErr;//12V异常
    bool cellFault;//电芯故障
    bool prechargeFault;//预充故障
    bool heatingFilmFault;//加热膜故障
    bool insulationBoardCommFault;//绝缘板通信故障
    bool samplingBoardCommFault;//采样板通信故障
    bool curDiverterFault;//电流分流器故障
    bool ntcFault;//NTC故障

    MSGPACK_DEFINE(chargeRelayCombine, chargeRelayDisabled, dischargeRelayCombine, dischargeRelayDisabled,
                    prechargeRelayCombine, prechargeRelayDisabled, negativeRelayCombine, negativeRelayDisabled,
                    heatingFilmRelayCombine, heatingFileRelayDisabled, _12VErr, cellFault, prechargeFault,
                    heatingFilmFault, insulationBoardCommFault, samplingBoardCommFault, curDiverterFault,
                    ntcFault);
};

struct BcuRelayRealStatus
{
    bool chargeRelay;// 充电继电器
    bool dischargeRelay;// 放电继电器
    bool prechargeRelay;// 预充继电器
    bool negativeRelay;// 负极继电器
    bool heatingFilmRelay;// 加热膜继电器

    MSGPACK_DEFINE(chargeRelay, dischargeRelay, prechargeRelay, negativeRelay, heatingFilmRelay)
};

struct BcuStatusSummary
{
    uint32_t protectStatusL1;// 1级保护状态
    uint32_t protectStatusL2;// 2级保护状态
    uint32_t protectStatusL3;// 3级保护状态
    uint32_t faultStatus;    // 故障状态
    uint32_t relayRealStatus;    // 继电器真实状态

    double volt;// 总电压
    double cur;// 总电流
    uint16_t soc;// SOC
    uint16_t soh;// SOH

    double cellvoltMax;    // 最大单体电压
    uint16_t cellvoltMaxAddr;// 最大单体电压地址

    double cellvoltMin;    // 最小单体电压
    uint16_t cellvoltMinAddr;// 最小单体电压地址

    double celltemMax;    // 最大单体温度
    uint16_t celltemMaxAddr;// 最大单体温度地址
    
    double celltemMin;    // 最小单体温度
    uint16_t celltemMinAddr;// 最小单体温度地址

    MSGPACK_DEFINE(protectStatusL1, protectStatusL2, protectStatusL3, faultStatus,
                    volt, cur, soc, soh,
                    cellvoltMax, cellvoltMaxAddr,
                    cellvoltMin, cellvoltMinAddr,
                    celltemMax, celltemMaxAddr,
                    celltemMin, celltemMinAddr);
};

struct BcuInfo
{
    /** 帧信息 */
    BcuStatusSummary base;

    /* 下一级信息: */
    map<int, BmuInfo> bmuList;

    /** 统计信息: */
    BcuWarningStatus warningStatus;
    BcuFaultStatus faultStatus;
    BcuRelayRealStatus relayRealStatus;
    bool existActiveWarningOrFault;
    int warningL1Count; // 1级告警数量
    int warningL2Count; // 2级告警数量
    int warningL3Count; // 3级告警数量
    int faultCount;     // 故障数量

    MSGPACK_DEFINE(base, bmuList, warningStatus, faultStatus, relayRealStatus, existActiveWarningOrFault,
                    warningL1Count, warningL2Count, warningL3Count, faultCount);
};

//////////////////////////////////////////////////////////////////////////////////

struct BauWarningStatus
{
    int cellVoltHigh;//单体高压
    int cellVoltLow;//单体低压
    int totalVoltHigh;//总体高压
    int totalVoltLow;//总体低压
    int chargeOverCur;//充电过流
    int dischargeOverCur;//放电过流
    int chargeTemHigh;//充电高温
    int dischargeTemHigh;//放电高温
    int chargeTemLow;//充电低温
    int dischargeTemLow;//放电低温
    int envTemHigh;//环境高温
    int envTemLow;//环境低温
    int chargeRelayTemHigh;//充电继电器高温
    int dischargeRelayTemHigh;//放电继电器高温
    int negativeRelayTemHigh;//负极继电器高温
    int socHigh;//SOC高
    int socLow;//SOC低
    int positiveInsulationLeakage;//正极绝缘漏电
    int negativeInsulationLeakage;//负极绝缘漏电
    int chargeVoltDiff;//充电压差
    int dischargeVoltDiff;//放电压差
    int chargeTemDiff;//充电温差
    int dischargeTemDiff;//放电温差
    int terminalTemHigh;//端子高温
    int bcuVoltDiff;//簇间压差

    MSGPACK_DEFINE(cellVoltHigh, cellVoltLow, totalVoltHigh, totalVoltLow,
                    chargeOverCur, dischargeOverCur, chargeTemHigh, dischargeTemHigh,
                    envTemHigh, envTemLow, chargeRelayTemHigh, dischargeRelayTemHigh,
                    negativeRelayTemHigh, socHigh, socLow, positiveInsulationLeakage,
                    negativeInsulationLeakage, chargeVoltDiff, dischargeVoltDiff,
                    chargeTemDiff, dischargeTemDiff, terminalTemHigh, bcuVoltDiff);
};

struct BauFaultStatus
{
    bool canBusErr;//CAN总线异常
    bool rs485Err;//RS485异常
    bool bcuVersionErr;//BCU版本异常
    bool bcuAddrErr;//BCU地址异常
    bool bcuOfflineErr;//BCU掉线异常

    MSGPACK_DEFINE(canBusErr, rs485Err, bcuVersionErr);
};

struct BauRunStatus
{
    bool idle;//空闲
    bool bcuCodeing;// BCU编码
    bool insulationCheck;//绝缘检测
    bool griding;//并机中
    bool charge;//充电
    bool discharge;//放电

    MSGPACK_DEFINE(idle, bcuCodeing, insulationCheck, griding, charge, discharge)
};

struct BauStatusSummary
{
    uint16_t systemStatus;    // 系统状态
    uint16_t bcuBingjiNum;//  簇并机数量
    uint32_t protectStatusL1;// 1级保护状态
    uint32_t protectStatusL2;// 2级保护状态
    uint32_t protectStatusL3;// 3级保护状态
    uint32_t faultStatus;    // 故障状态
    uint32_t runStatus;      // 系统状态

    double volt;// 总电压
    double cur;// 总电流
    uint16_t soc;// SOC
    uint16_t soh;// SOH

    double cellvoltMax;    // 最大单体电压
    uint16_t cellvoltMaxAddr;// 最大单体电压地址

    double cellvoltMin;    // 最小单体电压
    uint16_t cellvoltMinAddr;// 最小单体电压地址

    double celltemMax;    // 最大单体温度
    uint16_t celltemMaxAddr;// 最大单体温度地址
    
    double celltemMin;    // 最小单体温度
    uint16_t celltemMinAddr;// 最小单体温度地址
    
    double chargeCapacitySum;    // 累计充电电量
    double dischargeCapacitySum; // 累计放电电量

    double pcsRequestChargeCur;// PCS请求充电电流
    double pcsRequestChargeVolt;// PCS请求充电电压
    double pcsRequestDischargeCur;// PCS请求放电电流
    double pcsRequestDischargeVolt;// PCS请求放电电压

    double allowChargeCapacity;   // 可充电量
    double allowDischargeCapacity;// 可放电量

    uint16_t bcuNum;//BCU数量
    uint16_t bcuOnlineNum;//BCU在线数量

    uint16_t bcuSize;//单个BCU管理的BMU数量
    uint16_t bmuCellVoltSize;//单个BMU电芯电压数量
    uint16_t bmuCellTemSize;//单个BMU电芯温度数量
    uint16_t bmuTerminalTemSize;//单个BMU端子温度数量
    vector<int> bcuIndexListOnline;// 在线BCU编号列表

    MSGPACK_DEFINE(systemStatus, bcuBingjiNum, protectStatusL1, protectStatusL2, protectStatusL3, faultStatus,
                    volt, cur, soc, soh,
                    cellvoltMax, cellvoltMaxAddr,
                    cellvoltMin, cellvoltMinAddr,
                    celltemMax, celltemMaxAddr,
                    celltemMin, celltemMinAddr,
                    chargeCapacitySum, dischargeCapacitySum,
                    pcsRequestChargeCur, pcsRequestChargeVolt, pcsRequestDischargeCur, pcsRequestDischargeVolt,
                    allowChargeCapacity, allowDischargeCapacity,
                    bcuNum, bcuOnlineNum, bcuSize, bmuCellVoltSize, bmuCellTemSize, bmuTerminalTemSize, bcuIndexListOnline);
};

struct BingjiStatusSummary
{
    uint32_t protectStatusL1;// 1级保护状态
    uint32_t protectStatusL2;// 2级保护状态
    uint32_t protectStatusL3;// 3级保护状态
    uint32_t faultStatus;    // 故障状态
    uint32_t specialStatus;  // 特殊状态

    MSGPACK_DEFINE(protectStatusL1, protectStatusL2, protectStatusL3, faultStatus, specialStatus)
};

struct BauInfo
{
    BingjiStatusSummary bingjiStatusSummary;//帧信息
    BauStatusSummary bauStatusSummary;//帧信息

    map<int, BcuInfo> bcuList;//下一级信息

    /**
     * 统计信息:
     * 在帧信息及下一级信息的基础上，进一步生成统计信息。
    */
    BauWarningStatus warningStatus;
    BauFaultStatus faultStatus;
    BauRunStatus runStatus;
    int warningL1Count; // 1级告警数量
    int warningL2Count; // 2级告警数量
    int warningL3Count; // 3级告警数量
    int faultCount;     // 故障数量
    bool existActiveWarningOrFault;

    bool allowRunning;      // 可运行
    bool allowCharge;       // 可充电
    bool allowDischarge;    // 可放电

    MSGPACK_DEFINE(bingjiStatusSummary, bauStatusSummary, bcuList,
                    warningStatus, faultStatus, existActiveWarningOrFault,
                    warningL1Count, warningL2Count, warningL3Count, faultCount,
                    allowRunning, allowCharge, allowDischarge);
};

/////////////////////////////////////////////////////////////////////////////////////




}//namespace bau
}//namespace ems