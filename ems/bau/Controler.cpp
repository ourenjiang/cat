#include "Controler.h"
#include "boost/assert.hpp"
#include <string.h>
using namespace std;
using namespace ems::bau;

Controler::Controler()
    : log_(ems::Log4cppWrapper::getLogger(0))
    , isStableStatus_(false)
{
}

BatteryStatus Controler::getChargeStatus(const double realtimeVolt, const double realtimeCur, const double suggestVolt, const double suggestCur)
{
    const double diffVoltThreshold{ 5.0 };
    const double diffCurThreshold{ 5.0 };
    // log_.debug("电池电压: %.1f, 推荐充电电压: %.1f, 差值: %.1f", battery_.first, suggestCharge_.first, diffVolt);
    // log_.debug("电池电流(abs): %.1f, 推荐充电电流: %.1f, 差值: %.1f", abs(battery_.second), suggestCharge_.second, diffCur);

    if(realtimeVolt - suggestVolt > diffVoltThreshold || realtimeCur - suggestCur > diffCurThreshold){
        // log_.debug("充电状态1: 需要降低功率");
        isStableStatus_ = false;
        return BatteryStatus::SuggestDown;
    }

    if(suggestVolt - realtimeVolt > diffVoltThreshold && suggestCur - realtimeCur > diffCurThreshold)
    {
        // log_.debug("充电状态2: 允许提高功率");
        isStableStatus_ = false;
        return BatteryStatus::SuggestUp;
    }
    
    if(realtimeVolt > suggestVolt || realtimeCur > suggestCur)
    {
        // log_.debug("充电状态3: 需要稳定功率");
        isStableStatus_ = true;
        return BatteryStatus::Stable;
    }

    if(isStableStatus_)
    {
        // log_.debug("充电状态4: 需要稳定功率");
        return BatteryStatus::Stable;
    }
    // log_.debug("充电状态5: 允许提高功率");
    return BatteryStatus::SuggestUp;
}

BatteryStatus Controler::getDischargeStatus(const double volt, const double cur, const double suggestVolt, const double suggestCur)
{
    // 注意，沛城BAU中，充电电流为'正值'，放电电流为'负值'
    const double thresholdVolt{ 5.0 };
    const double thresholdCur{ 5.0 };
    const double diffVolt = volt - suggestVolt;
    const double diffCur = abs(cur) - suggestCur;// 放电电流为负值
    // log_.debug("电池电压: %.1f, 推荐放电电压: %.1f, 差值: %.1f", battery_.first, suggestDischarge_.first, diffVolt);
    // log_.debug("电池电流(abs): %.1f, 推荐放电电流: %.1f, 差值: %.1f", abs(battery_.second), suggestDischarge_.second, diffCur);

    if(diffVolt < thresholdVolt * (-1) || diffCur > thresholdCur)
    {
        // log_.debug("放电状态1: 需要降低功率");
        isStableStatus_ = false;
        return BatteryStatus::SuggestDown;
    }

    if(diffVolt > thresholdVolt && diffCur < thresholdCur * (-1))
    {
        // log_.debug("放电状态2: 允许提高功率");
        isStableStatus_ = false;
        return BatteryStatus::SuggestUp;
    }
    
    if(diffVolt < 0 || diffCur > 0)
    {
        // log_.debug("放电状态3: 需要稳定功率");
        isStableStatus_ = true;
        return BatteryStatus::Stable;
    }

    if(isStableStatus_)
    {
        // log_.debug("放电状态4: 需要稳定功率");
        return BatteryStatus::Stable;
    }
    // log_.debug("放电状态5: 允许提高功率");
    return BatteryStatus::SuggestUp;
}

bool Controler::verifyNormal(const uint32_t bauThirdProtectStatus, const uint32_t bauFaultStatus, const uint32_t bingjiThirdProtectStatus, const uint32_t bingjiFaultStatus)
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
bool Controler::verifyChargeAbility(const uint32_t bauThirdProtectStatus, const uint32_t bingjiThirdProtectStatus)
{
    if(!verifyChargeAbilityForBauThirdProtectStatus(bauThirdProtectStatus)) return false;
    if(!verifyChargeAbilityForBcuThirdProtectStatus(bingjiThirdProtectStatus)) return false;
    return true;
}

/*
检查是否有放电能力:
    与检查充电能力类似，需要验证BAU及其下管理的所有BCU的状态，所有校验都通过才表示'有放电能力'
*/
bool Controler::verifyDischargeAbility(const uint32_t bauThirdProtectStatus, uint32_t bingjiThirdProtectStatus)
{
    if(!verifyDischargeAbilityForBauThirdProtectStatus(bauThirdProtectStatus)) return false;
    if(!verifyDischargeAbilityForBcuThirdProtectStatus(bingjiThirdProtectStatus)) return false;
    return true;
}

bool Controler::verifyNormalForBauThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BauThirdProtectNormalMap) == sizeof(uint32_t));
    const BauThirdProtectNormalMap mapping{0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0};
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool Controler::verifyNormalForBauFaultStatus(const uint32_t status)
{
    static_assert(sizeof(BauFaultStatusMap) == sizeof(uint32_t));
    const BauFaultStatusMap mapping{ 1, 1, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool Controler::verifyNormalForBcuThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BcuThirdProtectNormalMap) == sizeof(uint32_t));
    const BcuThirdProtectNormalMap mapping{0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0};
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool Controler::verifyNormalForBcuFaultStatus(const uint32_t status)
{
    static_assert(sizeof(BcuFaultStatusMap) == sizeof(uint32_t));
    const BcuFaultStatusMap mapping{ 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool Controler::verifyChargeAbilityForBauThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BauThirdProtectChargeMap) == sizeof(uint32_t));
    const BauThirdProtectChargeMap mapping{ 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool Controler::verifyChargeAbilityForBcuThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BcuThirdProtectChargeMap) == sizeof(uint32_t));
    const BcuThirdProtectChargeMap mapping{ 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool Controler::verifyDischargeAbilityForBauThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BauThirdProtectDischargeMap) == sizeof(uint32_t));
    const BauThirdProtectDischargeMap mapping{ 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}

bool Controler::verifyDischargeAbilityForBcuThirdProtectStatus(const uint32_t status)
{
    static_assert(sizeof(BcuThirdProtectDischargeMap) == sizeof(uint32_t));
    const BcuThirdProtectDischargeMap mapping{ 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0 };
    
    uint32_t mappingBitSet;
    ::memcpy(&mappingBitSet, &mapping, sizeof(uint32_t));
    return !(mappingBitSet & status);
}
