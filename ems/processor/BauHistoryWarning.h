/**
 * 处理所有设备的告警变位记录的生成。
*/
#pragma once
#include "ems/bau/Model.h"
#include <bitset>
#include <unordered_map>

namespace ems
{
namespace processed
{
using namespace std;
using namespace std::placeholders;
using HistoryRecord = tuple<string, string, string, string, string, string>;

class BauHistoryWarning
{
public:
    BauHistoryWarning();
    void handleBranch(const int branchIndex, const bau::BauInfo& beforeInfo, const bau::BauInfo& currentInfo);
private:
    void handleBau(const string& devName, const bau::BauStatusSummary& beforeInfo, const bau::BauStatusSummary& currentInfo);
    void handleBcu(const string& devName, const bau::BcuStatusSummary& beforeInfo, const bau::BcuStatusSummary& currentInfo);
    void handleNewBcu(const string& devName, const bau::BcuStatusSummary& currentInfo);

    void handleBauProtectStatus(const string& devName, const string& level, const uint32_t before, const uint32_t current);
    void handleBauFaultStatus(const string& devName, const string& level, const uint32_t before, const uint32_t current);

    void handleBcuProtectStatus(const string& devName, const string& level, const uint32_t before, const uint32_t current);
    void handleBcuFaultStatus(const string& devName, const string& level, const uint32_t before, const uint32_t current);

    void save(const string& devName, const string& content, const bool before, const bool current, const string& level, const string& createTime);
};

}//namespace processed
}//namespace ems
