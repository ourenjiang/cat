#include "StorageEnergySystem.h"
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/base/StationInfo.h"
#include "utils/StreamWrapper.h"

using namespace ems;

void StorageEnergySystem::requestCallback(const httplib::Request &req, httplib::Response &res,
    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    auto stationInfo = base::getStationInfo(dataDealer);
    Json::Value root(Json::objectValue);
    root["trend"] = StorageEnergySystem::get_trend(stationInfo);

    // const auto& branchInfo = stationInfo.branchList[0];// 暂时只统计第一分支
    const auto& bauInfo = stationInfo.bauMap_[0];
    const auto& bauStatusSummary = bauInfo.bauStatusSummary;
    root["centralTopArea"] = StorageEnergySystem::get_centralTopArea(bauStatusSummary);
    root["branch"] = StorageEnergySystem::get_branch(stationInfo);

    const auto& pcsInfo = stationInfo.pcsMap_[0];
    root["warning"] = StorageEnergySystem::get_warning(bauInfo, pcsInfo);
    root["peak"] = StorageEnergySystem::get_peak(bauStatusSummary);

    // 成功响应
    Json::Value repJson;
    repJson["data"] = root;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);

}
catch(const std::exception& e){
    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

Json::Value StorageEnergySystem::get_trend(const StationInfo& stationInfo)
{
    Json::Value content;
    content["soc"] = get_trend_soc(stationInfo);
    content["cur"] = get_trend_cur(stationInfo);
    content["power"] = get_trend_power(stationInfo);
    return content;
}

Json::Value StorageEnergySystem::get_trend_soc(const StationInfo& stationInfo)
{
    Json::Value content;
    auto& snapshot = stationInfo.socRealtimeSnapshot;
    for(const auto& item: snapshot){
        Json::Value obj;
        obj["time"] = item.first;
        obj["value"] = stream_wrapper::serializeFloat(item.second, 1);
        content.append(obj);
    }
    return content;
}

Json::Value StorageEnergySystem::get_trend_cur(const StationInfo& stationInfo)
{
    Json::Value content;
    // auto& stroageInfo = storage_->getStationInfo();
    auto& snapshot = stationInfo.curRealtimeSnapshot;
    for(const auto& item: snapshot){
        Json::Value obj;
        obj["time"] = item.first;
        obj["value"] = stream_wrapper::serializeFloat(item.second, 1);
        content.append(obj);
    }
    return content;
}

Json::Value StorageEnergySystem::get_trend_power(const StationInfo& stationInfo)
{
    Json::Value content;
    // auto& stroageInfo = storage_->getStationInfo();
    auto& snapshot = stationInfo.powerRealtimeSnapshot;
    for(const auto& item: snapshot){
        Json::Value obj;
        obj["time"] = item.first;
        obj["value"] = stream_wrapper::serializeFloat(item.second, 1);
        content.append(obj);
    }
    return content;
}

Json::Value StorageEnergySystem::get_centralTopArea(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    content["totalVolt"] = stream_wrapper::serializeFloat(bauStatusSummary.volt, 2);
    content["totalCur"] = stream_wrapper::serializeFloat(bauStatusSummary.cur, 2);
    {
        const double totalPower = (bauStatusSummary.volt) * (bauStatusSummary.cur);
        const double totalPowerKw = totalPower * 0.001;
        content["totalPower"] = stream_wrapper::serializeFloat(totalPowerKw, 2);
    }
    content["allowChargeCapacity"] = stream_wrapper::serializeFloat(bauStatusSummary.allowChargeCapacity, 2);
    content["allowDischargeCapacity"] = stream_wrapper::serializeFloat(bauStatusSummary.allowDischargeCapacity, 2);
    content["heapNum"] = "1";
    return content;
}

Json::Value StorageEnergySystem::get_branch(const StationInfo& stationInfo)
{
    Json::Value content;
    const int branchNum = 1;//暂时写死

    for(int index = 0; index < branchNum; ++index){
        Json::Value branch;

        // const auto& branchInfo = itr->second;
        const auto& pcsInfo = stationInfo.pcsMap_.at(index);
        branch["pcs"] = get_branch_pcs(pcsInfo.frame_0406_0460_summary);

        const auto& bauInfo = stationInfo.bauMap_.at(index);
        branch["bau"] = get_branch_bau(bauInfo.bauStatusSummary);
        content.append(branch);
    }
    return content;
}

Json::Value StorageEnergySystem::get_branch_pcs(const pcs::_0406_0460_Summary& summary)
{
    Json::Value content;
    
    content["status"] = "Reversed";

    {
        const string gridLineVoltAB = stream_wrapper::serializeFloat(summary.gridLineVoltAB, 1);
        const string gridLineVoltBC = stream_wrapper::serializeFloat(summary.gridLineVoltBC, 1);
        const string gridLineVoltCA = stream_wrapper::serializeFloat(summary.gridLineVoltCA, 1);
        content["volt"] = gridLineVoltAB + '/' + gridLineVoltBC + '/' + gridLineVoltCA;
    }
    {
        const string gridCurA = stream_wrapper::serializeFloat(summary.gridCurA, 1);
        const string gridCurB = stream_wrapper::serializeFloat(summary.gridCurB, 1);
        const string gridCurC = stream_wrapper::serializeFloat(summary.gridCurC, 1);
        content["cur"] = gridCurA + '/' + gridCurB + '/' + gridCurC;
    }
    {
        const string gridTotalActivePower = stream_wrapper::serializeFloat(summary.gridTotalActivePower, 1);
        const string gridTotalReactivePower = stream_wrapper::serializeFloat(summary.gridTotalReactivePower, 1);
        const string gridTotalApparentPower = stream_wrapper::serializeFloat(summary.gridTotalApparentPower, 1);
        content["power"] = gridTotalActivePower + '/' + gridTotalReactivePower + '/' + gridTotalApparentPower;
    }
    return content;
}

Json::Value StorageEnergySystem::get_branch_bau(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    content["soc"] = stream_wrapper::serializeFloat(bauStatusSummary.soc, 0);
    content["soh"] = stream_wrapper::serializeFloat(bauStatusSummary.soh, 0);
    content["volt"] = stream_wrapper::serializeFloat(bauStatusSummary.volt, 2);
    content["cur"] = stream_wrapper::serializeFloat(bauStatusSummary.cur, 2);
    {
        const double power = (bauStatusSummary.volt) * (bauStatusSummary.cur);
        const double powerKw = power * 0.001;
        content["power"] = stream_wrapper::serializeFloat(powerKw, 2);
    }
    content["cellvoltMax"] = stream_wrapper::serializeFloat(bauStatusSummary.cellvoltMax, 3);
    content["cellvoltMin"] = stream_wrapper::serializeFloat(bauStatusSummary.cellvoltMin, 3);
    content["celltemMax"] = stream_wrapper::serializeFloat(bauStatusSummary.celltemMax, 2);
    content["celltemMin"] = stream_wrapper::serializeFloat(bauStatusSummary.celltemMin, 2);
    return content;
}

Json::Value StorageEnergySystem::get_warning(const bau::BauInfo& bauInfo, const pcs::PcsInfo& pcsInfo)
{
    Json::Value content;
    {
        const int bmsWarningTotal = bauInfo.warningL1Count + bauInfo.warningL2Count + bauInfo.warningL3Count;
        const int pcsWarningTotal = pcsInfo.warningL1Count + pcsInfo.warningL2Count + pcsInfo.warningL3Count;
        content["currentNum"] = to_string(bmsWarningTotal + pcsWarningTotal);
    }
    {
        content["todayAddNum"] = "Reserved";
    }
    {
        const int level1WarningTotal = bauInfo.warningL1Count + pcsInfo.warningL1Count;
        const int level2WarningTotal = bauInfo.warningL2Count + pcsInfo.warningL2Count;
        const int level3WarningTotal = bauInfo.warningL3Count + pcsInfo.warningL3Count;
        content["level1"] = to_string(level1WarningTotal);
        content["level2"] = to_string(level2WarningTotal);
        content["level3"] = to_string(level3WarningTotal);
    }
    return content;
}

string StorageEnergySystem::convertCellAddrFormat(const uint16_t cellGlobalIndex)
{
    const int bcuCapacity{ 20 };
    const int bmuCapacity{ 64 };
    const int bcuIndex{ cellGlobalIndex / (bcuCapacity * bmuCapacity) };
    const int bmuIndex{ cellGlobalIndex % (bcuCapacity * bmuCapacity) / bmuCapacity };
    const int cellIndex{ cellGlobalIndex % bmuCapacity };
    return to_string(bcuIndex + 1)
           + '/' + to_string(bmuIndex + 1)
           + '/' + to_string(cellIndex + 1);
}

Json::Value StorageEnergySystem::get_peak(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    {
        auto& obj = content["cellvoltMax"];
        obj["value"] = stream_wrapper::serializeFloat(bauStatusSummary.cellvoltMax, 3);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.cellvoltMaxAddr);
    }
    {
        auto& obj = content["cellvoltMin"];
        obj["value"] = stream_wrapper::serializeFloat(bauStatusSummary.cellvoltMin, 3);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.cellvoltMinAddr);
    }
    {
        auto& obj = content["celltemMax"];
        obj["value"] = stream_wrapper::serializeFloat(bauStatusSummary.celltemMax, 1);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.celltemMaxAddr);
    }
    {
        auto& obj = content["celltemMin"];
        obj["value"] = stream_wrapper::serializeFloat(bauStatusSummary.celltemMin, 1);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.celltemMinAddr);
    }
    return content;
}
