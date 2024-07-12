#include "DeviceStatus.h"
#include "boost/assert.hpp"
#include <string.h>
#include <math.h>

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

bool DeviceStatus::verifyNormal(const uint32_t bauThirdProtectStatus, const uint32_t bauFaultStatus, const uint32_t bingjiThirdProtectStatus, const uint32_t bingjiFaultStatus)
{
    if(!verifyNormalForBauThirdProtectStatus(bauThirdProtectStatus)) return false;
    if(!verifyNormalForBauFaultStatus(bauFaultStatus)) return false;
    if(!verifyNormalForBcuThirdProtectStatus(bingjiThirdProtectStatus)) return false;
    if(!verifyNormalForBcuFaultStatus(bingjiFaultStatus)) return false;
    return true;
}

/*
检查是否有充电能力:
    注意，该动作应该在 verifyNormal()返回为真的条件之下，才有必要执行；
    先检查BAU这一级的三级保护状态中所有属于'充电故障'的标志位；
    再遍历BAU下面的所有BCU的三级保护状态中所有属于'充电故障'的标志位；
    以上所有均验证通过，才表示'有充电能力'
*/
bool DeviceStatus::verifyChargeAbility(const uint32_t bauThirdProtectStatus, const uint32_t bingjiThirdProtectStatus)
{
    if(!verifyChargeAbilityForBauThirdProtectStatus(bauThirdProtectStatus)) return false;
    if(!verifyChargeAbilityForBcuThirdProtectStatus(bingjiThirdProtectStatus)) return false;
    return true;
}

/*
检查是否有放电能力:
    与检查充电能力类似，需要验证BAU及其下管理的所有BCU的状态，所有校验都通过才表示'有放电能力'
*/
bool DeviceStatus::verifyDischargeAbility(const uint32_t bauThirdProtectStatus, uint32_t bingjiThirdProtectStatus)
{
    if(!verifyDischargeAbilityForBauThirdProtectStatus(bauThirdProtectStatus)) return false;
    if(!verifyDischargeAbilityForBcuThirdProtectStatus(bingjiThirdProtectStatus)) return false;
    return true;
}

bool DeviceStatus::verifyNormalForBauThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BauThirdProtectNormalMap) == sizeof(uint32_t));
    const BauThirdProtectNormalMap mapping{0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0};
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool DeviceStatus::verifyNormalForBauFaultStatus(const uint32_t status)
{
    static_assert(sizeof(BauFaultStatusMap) == sizeof(uint32_t));
    const BauFaultStatusMap mapping{ 1, 1, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool DeviceStatus::verifyNormalForBcuThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BcuThirdProtectNormalMap) == sizeof(uint32_t));
    const BcuThirdProtectNormalMap mapping{0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0};
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool DeviceStatus::verifyNormalForBcuFaultStatus(const uint32_t status)
{
    static_assert(sizeof(BcuFaultStatusMap) == sizeof(uint32_t));
    const BcuFaultStatusMap mapping{ 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool DeviceStatus::verifyChargeAbilityForBauThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BauThirdProtectChargeMap) == sizeof(uint32_t));
    const BauThirdProtectChargeMap mapping{ 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool DeviceStatus::verifyChargeAbilityForBcuThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BcuThirdProtectChargeMap) == sizeof(uint32_t));
    const BcuThirdProtectChargeMap mapping{ 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool DeviceStatus::verifyDischargeAbilityForBauThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BauThirdProtectDischargeMap) == sizeof(uint32_t));
    const BauThirdProtectDischargeMap mapping{ 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool DeviceStatus::verifyDischargeAbilityForBcuThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BcuThirdProtectDischargeMap) == sizeof(uint32_t));
    const BcuThirdProtectDischargeMap mapping{ 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}
