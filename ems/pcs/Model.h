#pragma once
#include "msgpack.hpp"

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

    MSGPACK_DEFINE(warning1_invertOverCur, warning1_batteryVoltLow, warning1_batteryChargeDisabled,
                    warning1_dcGeneratrixOverVolt, warning1_dcGeneratrixShortCircuit,
                    warning1_outputContactorOpenCircuit, warning1_outputContactorShortCircuit,
                    warning1_converterOverTem, warning1_outputOverLoad,
                    warning2_gridOverVolt, warning2_gridLackVolt, warning2_gridPhaseOrderReverse,
                    warning2_gridIslandingEffectProtect, warning2_batteryDischargeDisabled,
                    warning2_energentPowerOff, warning2_converterNotSync);
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

    MSGPACK_DEFINE(warning1_zbLimitCurFault, warning1_converterFault, warning1_bingjiCommFault,
                    warning1_batteryConnectReverse, warning1_dcContactorFault, warning1_bmsCommFault,
                    warning1_inverterLackPhaseFault, warning2_gridFrequencyErr, warning2_drivingLineFault,
                    warning2_lightningProtectFault, warning2_insulationImpedanceErr, warning2_invertOverVoltFault,
                    warning2_15VPowerFault, warning2_acFanFault, warning2_batteryFault, warning2_ctOrHallOpenCircuitFault);
};

struct RunStatus
{
    // 寄存器 1032
    bool faultTotal;// 故障总
    bool warnTotal;// 告警总
    bool powerTotal;// 开机状态总

    // 寄存器 1033
    bool dcInputBreaker;// 直流输入断路器
    bool dcContactor;// 直流接触状态
    bool outputBreaker;// 输出断路器状态
    bool outputContactor;// 输出接触器状态
    bool gridBreaker;// 输出断路器状态
    bool gridContactor;// 输出接触器状态

    // 寄存器 1034
    bool gridOnCharge;// 并网充电
    bool gridOnDischarge;// 并网放电
    bool standby;// 待机
    MSGPACK_DEFINE(faultTotal, warnTotal, powerTotal,
                    dcInputBreaker, dcContactor, outputBreaker, outputContactor, gridBreaker, gridContactor,
                    gridOnCharge, gridOnDischarge, standby);
};

struct _0406_0460_Summary
{
    uint16_t warnStatus1;      // 告警字1
    uint16_t warnStatus2;      // 告警字2
    uint16_t warnStatus3;      // 告警字3
    uint16_t switchStatus;     // 开关状态字
    uint16_t pcsStatus;        // PCS状态字
    uint16_t perihperalStatus; // 外设状态

    double gridLineVoltAB;  // 电网线电压AB
    double gridLineVoltBC;  // 电网线电压BC
    double gridLineVoltCA;  // 电网线电压CA
    double gridCurA;  // 电网电流A
    double gridCurB;  // 电网电流B
    double gridCurC;  // 电网电流C
    double gridTotalActivePower;  // 电网总有功功率
    double gridTotalReactivePower;  // 电网总无功功率
    double gridTotalApparentPower;  // 电网总视在功率

    MSGPACK_DEFINE(warnStatus1, warnStatus2, warnStatus3, switchStatus, pcsStatus, perihperalStatus,
                    gridLineVoltAB, gridLineVoltBC, gridLineVoltCA,
                    gridCurA, gridCurB, gridCurC,
                    gridTotalActivePower, gridTotalReactivePower, gridTotalApparentPower);
};

struct _0474_04D0_Summary
{
    double activePowerSetting; // 输出有功功率设置
    uint16_t remoteMode; // 调度方式（0本地，1远程)
    uint16_t powerSetting; // 遥控开关机设置(0x5555 - 变流器开机, 0xAAAA - 变流器关机, 0x55AA - 变流器待机)

    MSGPACK_DEFINE(activePowerSetting, remoteMode, powerSetting);
};

struct PcsInfo
{
    pcs::_0406_0460_Summary frame_0406_0460_summary;//帧汇总信息
    pcs::_0474_04D0_Summary frame_0474_04D0_summary;//帧汇总信息

    // 汇总信息
    WarningStatus warningStatus;
    FaultStatus faultStatus;
    RunStatus runStatus;
    bool existActiveWarningOrFault;
    int warningL1Count; // 1级告警数量
    int warningL2Count; // 2级告警数量
    int warningL3Count; // 3级告警数量
    int faultCount;     // 故障数量
    
    MSGPACK_DEFINE(frame_0406_0460_summary, frame_0474_04D0_summary,
                    warningStatus, faultStatus, runStatus, existActiveWarningOrFault,
                    warningL1Count, warningL2Count, warningL3Count, faultCount);
};
}//namespace station
}//namespace ems
