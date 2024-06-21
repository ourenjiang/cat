#pragma once
#include <memory>
#include "PollForward.h"
#include "Poller.h"

namespace ems
{
namespace pcs
{
using namespace std;

struct WarningStatus
{
    /** 告警字1 */
    bool warning1_invertOverCur;   //逆变过流
    // bool warning1_zbLimitCurFault; //逐波限流故障
    // bool warning1_converterFault; //变流器故障
    bool warning1_batteryVoltLow; //电池电压低
    bool warning1_batteryChargeDisabled; //电池充电不允许
    // bool warning1_bingjiCommFault; //并机通信故障
    bool warning1_dcGeneratrixOverVolt; //直流母线过压
    bool warning1_dcGeneratrixShortCircuit; //直流母线短路
    bool warning1_outputContactorOpenCircuit; //输出接触器开路
    bool warning1_outputContactorShortCircuit; //输出接触器短路
    bool warning1_converterOverTem; //变流器过温
    bool warning1_outputOverLoad; //输出过载
    // bool warning1_batteryConnectReverse; //电池接反故障
    // bool warning1_dcContactorFault; //DC接触器故障
    // bool warning1_bmsCommFault; //BMS通信故障
    // bool warning1_inverterLackPhaseFault; //逆变器缺相故障

    /** 告警字2 */
    bool warning2_gridOverVolt; //电网过压
    bool warning2_gridLackVolt; //电网欠压
    bool warning2_gridPhaseOrderReverse; //电网相序反
    // bool warning2_gridFrequencyErr; //电网频率异常
    bool warning2_gridIslandingEffectProtect; //电网孤岛效应保护
    // bool warning2_drivingLineFault; //驱动线故障
    // bool warning2_lightningProtectFault; //防雷故障
    // bool warning2_insulationImpedanceErr; //绝缘阻抗异常
    bool warning2_batteryDischargeDisabled; //电池放电不允许
    // bool warning2_invertOverVoltFault; //逆变过压故障
    // bool warning2_15VPowerFault; //15V电源故障
    // bool warning2_acFanFault; //交流风扇故障
    // bool warning2_batteryFault; //电池故障
    bool warning2_energentPowerOff; //紧急关机
    bool warning2_converterNotSync; //变流器不同步
    // bool warning2_ctOrHallOpenCircuitFault; //CT或霍尔开路故障
};

struct FaultStatus
{
    bool warning1_zbLimitCurFault; //逐波限流故障
    bool warning1_converterFault; //变流器故障
    bool warning1_bingjiCommFault; //并机通信故障
    bool warning1_batteryConnectReverse; //电池接反故障
    bool warning1_dcContactorFault; //DC接触器故障
    bool warning1_bmsCommFault; //BMS通信故障
    bool warning1_inverterLackPhaseFault; //逆变器缺相故障
    bool warning2_gridFrequencyErr; //电网频率异常
    bool warning2_drivingLineFault; //驱动线故障
    bool warning2_lightningProtectFault; //防雷故障
    bool warning2_insulationImpedanceErr; //绝缘阻抗异常
    bool warning2_invertOverVoltFault; //逆变过压故障
    bool warning2_15VPowerFault; //15V电源故障
    bool warning2_acFanFault; //交流风扇故障
    bool warning2_batteryFault; //电池故障
    bool warning2_ctOrHallOpenCircuitFault; //CT或霍尔开路故障
};

struct PcsInfo
{
    PcsInfo()
        : onlineFlag(false)
    {
    }

    // 在线标志
    bool onlineFlag;
    pcs::PollForward pollForward;//轮询代理
    pcs::Poller pcsPoller;// 帧轮询器
    pcs::_0406_0460_Summary frame_0406_0460_summary;//帧汇总信息
    pcs::_0474_04D0_Summary frame_0474_04D0_summary;//帧汇总信息

    // 汇总信息
    WarningStatus warningStatus;
    FaultStatus faultStatus;
    bool existActiveWarningOrFault;
    int warningL1Count; // 1级告警数量
    int warningL2Count; // 2级告警数量
    int warningL3Count; // 3级告警数量
    int faultCount;     // 故障数量
};
}//namespace station
}//namespace ems
