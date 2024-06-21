#pragma once
#include "BauPoller.h"
#include "BcuPoller.h"
#include "BmuPoller.h"
#include "PollForward.h"
#include <iostream>

namespace ems
{
namespace bau
{
using namespace std;

struct BmuInfo
{
    /**
     * 基础信息:
    */
    CellvoltSummary cellvoltInfo;
    CelltemSummary celltemInfo;

    /* 统计信息 */
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
};

struct BcuInfo
{
    BcuInfo()
        : onlineFlag(false)
    {
        cout << "call BcuInfo()" << endl;
    }

    // 在线标志
    bool onlineFlag;

    /** 帧信息 */
    BcuStatusSummary base;

    /* 下一级信息: */
    map<int, BmuInfo> bmuList;

    /** 统计信息: */
    BcuWarningStatus warningStatus;
    BcuFaultStatus faultStatus;
    bool existActiveWarningOrFault;
    int warningL1Count; // 1级告警数量
    int warningL2Count; // 2级告警数量
    int warningL3Count; // 3级告警数量
    int faultCount;     // 故障数量
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
};

struct BauFaultStatus
{
    bool canBusErr;//CAN总线异常
    bool rs485Err;//RS485异常
    bool bcuVersionErr;//BCU版本异常
};

struct BauInfo
{
    BauInfo()
        : onlineFlag(false)
        , initFlag(false)
    {
    }

    bool onlineFlag;// 在线标志
    bool initFlag;// 在线标志

    bau::PollForward pollForward;//轮询转发
    BauPoller bauPoller;//帧轮询器
    BcuPoller bcuPoller;//帧轮询器
    BmuPoller bmuPoller;//帧轮询器

    BingjiStatusSummary bingjiStatusSummary;//帧信息
    BauStatusSummary bauStatusSummary;//帧信息

    map<int, BcuInfo> bcuList;//下一级信息

    /**
     * 统计信息:
     * 在帧信息及下一级信息的基础上，进一步生成统计信息。
    */
    BauWarningStatus warningStatus;
    BauFaultStatus faultStatus;
    bool existActiveWarningOrFault;
    int warningL1Count; // 1级告警数量
    int warningL2Count; // 2级告警数量
    int warningL3Count; // 3级告警数量
    int faultCount;     // 故障数量
};

/////////////////////////////////////////////////////////////////////////////////////




}//namespace bau
}//namespace ems