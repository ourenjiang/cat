#pragma once
#include <stdint.h>

namespace ems
{
namespace bau
{
using namespace std;

enum class BatteryStatus
{
    Null,           // 空
    Stable,         // 稳定输出
    SuggestUp,      // 建议提高输出功率
    SuggestDown,    // 建议降低输出功率
};

struct BauFaultStatusMap
{
    uint32_t bit_00:1;//CAN总线异常
    uint32_t bit_01:1;//RS485异常
    uint32_t bit_02:1;//BCU版本异常
    uint32_t bit_03_31:29;
};

struct BcuFaultStatusMap
{
    uint32_t bit_00:1;// 充电继电器粘连
    uint32_t bit_01:1;// 充电继电器失效
    uint32_t bit_02:1;// 放电继电器粘连
    uint32_t bit_03:1;// 放电继电器失效
    uint32_t bit_04:1;// 预充继电器粘连
    uint32_t bit_05:1;// 预充继电器失效
    uint32_t bit_06:1;// 负极继电器粘连
    uint32_t bit_07:1;// 负极继电器失效
    uint32_t bit_08_10:3;
    uint32_t bit_11:1;// 电芯故障
    uint32_t bit_12:1;// 预充故障
    uint32_t bit_13:1;
    uint32_t bit_14:1;// 绝缘板通信故障
    uint32_t bit_15:1;// 采样板通信故障
    uint32_t bit_16:1;
    uint32_t bit_17:1;// NTC故障
    uint32_t bit_18_31:14;
};

struct BauThirdProtectNormalMap
{
    uint32_t bit_00_09:10;
    uint32_t bit_10:1;// 环境高温
    uint32_t bit_11:1;// 环境低温
    uint32_t bit_12_13:2;
    uint32_t bit_14:1;// 负极继电器高温
    uint32_t bit_15_16:2;
    uint32_t bit_17:1;// 正极绝缘漏电
    uint32_t bit_18:1;// 负极绝缘漏电
    uint32_t bit_19_22:4;
    uint32_t bit_23:1;// 端子高温
    uint32_t bit_24:1;// 簇间压差
    uint32_t bit_25_31:7;
};

struct BauThirdProtectChargeMap
{
    uint32_t bit_00:1;// 单体高压
    uint32_t bit_01:1;
    uint32_t bit_02:1;// 总体高压
    uint32_t bit_03:1;
    uint32_t bit_04:1;// 充电过流
    uint32_t bit_05:1;
    uint32_t bit_06:1;// 充电高温
    uint32_t bit_07:1;
    uint32_t bit_08:1;// 充电低温
    uint32_t bit_09_11:3;
    uint32_t bit_12:1;// 充电继电器高温
    uint32_t bit_13_14:2;
    uint32_t bit_15:1;// SOC高
    uint32_t bit_16_18:3;
    uint32_t bit_19:1;// 充电压差
    uint32_t bit_20:1;
    uint32_t bit_21:1;// 充电温差
    uint32_t bit_22_31:10;
};

struct BcuThirdProtectChargeMap
{
    uint32_t bit_00:1;// 单体高压
    uint32_t bit_01:1;
    uint32_t bit_02:1;// 总体高压
    uint32_t bit_03:1;
    uint32_t bit_04:1;// 充电过流
    uint32_t bit_05:1;
    uint32_t bit_06:1;// 充电高温
    uint32_t bit_07:1;
    uint32_t bit_08:1;// 充电低温
    uint32_t bit_09_11:3;
    uint32_t bit_12:1;// 充电继电器高温
    uint32_t bit_13_14:2;
    uint32_t bit_15:1;// SOC高
    uint32_t bit_16_18:3;
    uint32_t bit_19:1;// 充电压差
    uint32_t bit_20:1;
    uint32_t bit_21:1;// 充电温差
    uint32_t bit_22_31:10;
};

struct BauThirdProtectDischargeMap
{
    uint32_t bit_00:1;
    uint32_t bit_01:1;// 单体低压
    uint32_t bit_02:1;
    uint32_t bit_03:1;// 总体低压
    uint32_t bit_04:1;
    uint32_t bit_05:1;// 放电过流
    uint32_t bit_06:1;
    uint32_t bit_07:1;// 放电高温
    uint32_t bit_08:1;
    uint32_t bit_09:1;// 放电低温
    uint32_t bit_10_12:3;
    uint32_t bit_13:1;// 放电继电器高温
    uint32_t bit_14_15:2;
    uint32_t bit_16:1;// SOC低
    uint32_t bit_17_19:3;
    uint32_t bit_20:1;// 放电压差
    uint32_t bit_21:1;
    uint32_t bit_22:1;// 放电温差
    uint32_t bit_23_31:9;
};

struct BcuThirdProtectDischargeMap
{
    uint32_t bit_00:1;
    uint32_t bit_01:1;// 单体低压
    uint32_t bit_02:1;
    uint32_t bit_03:1;// 总体低压
    uint32_t bit_04:1;
    uint32_t bit_05:1;// 放电过流
    uint32_t bit_06:1;
    uint32_t bit_07:1;// 放电高温
    uint32_t bit_08:1;
    uint32_t bit_09:1;// 放电低温
    uint32_t bit_10_12:3;
    uint32_t bit_13:1;// 放电继电器高温
    uint32_t bit_14_15:2;
    uint32_t bit_16:1;// SOC低
    uint32_t bit_17_19:3;
    uint32_t bit_20:1;// 放电压差
    uint32_t bit_21:1;
    uint32_t bit_22:1;// 放电温差
    uint32_t bit_23_31:9;
};

struct BcuThirdProtectNormalMap
{
    uint32_t bit_00_09:10;
    uint32_t bit_10:1;// 环境高温
    uint32_t bit_11:1;// 环境低温
    uint32_t bit_12_13:2;
    uint32_t bit_14:1;// 负极继电器高温
    uint32_t bit_15_16:2;
    uint32_t bit_17:1;// 正极绝缘漏电
    uint32_t bit_18:1;// 负极绝缘漏电
    uint32_t bit_19_22:4;
    uint32_t bit_23:1;// 电芯升温
    uint32_t bit_24:1;// 电芯采样
    uint32_t bit_25:1;// NTC采样异常
    uint32_t bit_26:1;// 端子高温
    uint32_t bit_27_31:5;
};

class DeviceStatus
{
public:
    DeviceStatus();
    BatteryStatus getChargeStatus(const double volt, const double cur, const double suggestVolt, const double suggestCur);// 获取充电状态
    BatteryStatus getDischargeStatus(const double volt, const double cur, const double suggestVolt, const double suggestCur);// 获取放电状态
    bool verifyNormal(const uint32_t bauThirdProtectStatus, const uint32_t bauFaultStatus, const uint32_t bingjiThirdProtectStatus, const uint32_t bingjiFaultStatus);
    bool verifyChargeAbility(const uint32_t bauThirdProtectStatus, const uint32_t bingjiThirdProtectStatus);// 检查是否有充电能力
    bool verifyDischargeAbility(const uint32_t bauThirdProtectStatus, const uint32_t bingjiThirdProtectStatus);// 检查是否有放电能力
private:
    bool verifyNormalForBauThirdProtectStatus(const uint32_t status);
    bool verifyNormalForBauFaultStatus(const uint32_t status);
    bool verifyNormalForBcuThirdProtectStatus(const uint32_t status);
    bool verifyNormalForBcuFaultStatus(const uint32_t status);

    bool verifyChargeAbilityForBauThirdProtectStatus(const uint32_t status);
    bool verifyChargeAbilityForBcuThirdProtectStatus(const uint32_t status);

    bool verifyDischargeAbilityForBauThirdProtectStatus(const uint32_t status);
    bool verifyDischargeAbilityForBcuThirdProtectStatus(const uint32_t status);
    bool isStableStatus_;// 当前是否为稳定状态
};
}//namespace bau
}//namespace ems
