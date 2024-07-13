#include "DeviceStatus.h"
#include "boost/assert.hpp"
#include <string.h>
#include <math.h>
#include <bitset>

using namespace std;
using namespace ems::bau;

DeviceStatus::DeviceStatus()
    : isStableStatus_(false)
{
}

BatteryStatus DeviceStatus::getChargeStatus(const double realtimeVolt, const double realtimeCur, const double suggestVolt, const double suggestCur)
{
    const double diffVoltThreshold{ 5.0 };
    const double diffCurThreshold{ 5.0 };

    if(realtimeVolt - suggestVolt > diffVoltThreshold || realtimeCur - suggestCur > diffCurThreshold){
        isStableStatus_ = false;
        return BatteryStatus::SuggestDown;
    }

    if(suggestVolt - realtimeVolt > diffVoltThreshold && suggestCur - realtimeCur > diffCurThreshold)
    {
        isStableStatus_ = false;
        return BatteryStatus::SuggestUp;
    }
    
    if(realtimeVolt > suggestVolt || realtimeCur > suggestCur)
    {
        isStableStatus_ = true;
        return BatteryStatus::Stable;
    }

    if(isStableStatus_)
    {
        return BatteryStatus::Stable;
    }
    return BatteryStatus::SuggestUp;
}

BatteryStatus DeviceStatus::getDischargeStatus(const double volt, const double cur, const double suggestVolt, const double suggestCur)
{
    // 注意，沛城BAU中，充电电流为'正值'，放电电流为'负值'
    const double thresholdVolt{ 5.0 };
    const double thresholdCur{ 5.0 };
    const double diffVolt = volt - suggestVolt;
    const double diffCur = ::fabs(cur) - suggestCur;// 放电电流为负值

    if(diffVolt < thresholdVolt * (-1) || diffCur > thresholdCur)
    {
        isStableStatus_ = false;
        return BatteryStatus::SuggestDown;
    }

    if(diffVolt > thresholdVolt && diffCur < thresholdCur * (-1))
    {
        isStableStatus_ = false;
        return BatteryStatus::SuggestUp;
    }
    
    if(diffVolt < 0 || diffCur > 0)
    {
        isStableStatus_ = true;
        return BatteryStatus::Stable;
    }

    if(isStableStatus_)
    {
        return BatteryStatus::Stable;
    }
    return BatteryStatus::SuggestUp;
}
