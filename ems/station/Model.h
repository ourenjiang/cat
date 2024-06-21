#pragma once
#include <map>
#include <deque>
#include "ems/bau/Model.h"
#include "ems/pcs/Model.h"
#include "ems/xftg/StrategyXftg.h"

namespace ems
{
using namespace std;

struct BranchInfo
{
    bau::BauInfo bauInfo;
    pcs::PcsInfo pcsInfo;
    xftg::StrategyXftg xftgStrategy;
};

struct StationInfo
{
    map<int, shared_ptr<BranchInfo>> branchList;//基础信息

    /* 统计信息 */
    deque<pair<string, double>> socRealtimeSnapshot;// SOC实时曲线
    deque<pair<string, double>> curRealtimeSnapshot;// 电流实时曲线
    deque<pair<string, double>> powerRealtimeSnapshot;// 功率实时曲线
};
}//namespace ems
