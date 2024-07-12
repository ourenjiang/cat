#pragma once
#include <map>
#include <deque>
// #include "ems/branch/Model.h"
#include "ems/bau/Model.h"
#include "ems/pcs/Model.h"
#include "ems/xftg/Model.h"

namespace ems
{
using namespace std;

struct StationInfo
{
    // map<int, shared_ptr<BranchInfo>> branchList;//基础信息
    map<int, bau::BauInfo> bauMap_;
    map<int, pcs::PcsInfo> pcsMap_;
    map<int, xftg::StrategyInfo> strategyMap_;

    /* 统计信息 */
    deque<pair<string, double>> socRealtimeSnapshot;// SOC实时曲线
    deque<pair<string, double>> curRealtimeSnapshot;// 电流实时曲线
    deque<pair<string, double>> powerRealtimeSnapshot;// 功率实时曲线
    
    MSGPACK_DEFINE(bauMap_, pcsMap_, strategyMap_,
        socRealtimeSnapshot, curRealtimeSnapshot, powerRealtimeSnapshot);
};
}//namespace ems
