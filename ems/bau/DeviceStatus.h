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

class DeviceStatus
{
public:
    DeviceStatus();
    BatteryStatus getChargeStatus(const double volt, const double cur, const double suggestVolt, const double suggestCur);// 获取充电状态
    BatteryStatus getDischargeStatus(const double volt, const double cur, const double suggestVolt, const double suggestCur);// 获取放电状态
    
private:
    bool isStableStatus_;// 当前是否为稳定状态
};

}//namespace bau
}//namespace ems
