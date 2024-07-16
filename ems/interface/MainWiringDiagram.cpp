#include "MainWiringDiagram.h"
#include "boost/assert.hpp"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/base/StationInfo.h"
#include "utils/StreamWrapper.h"

using namespace ems;

void MainWiringDiagram::requestCallback(const httplib::Request &req, httplib::Response &res,
    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    auto stationInfo = base::getStationInfo(dataDealer);

    Json::Value root;
    root["realPower"] = get_realPower(stationInfo);

    // const auto& branchInfo = stationInfo.branchList[0];// 暂时只统计第一分支
    const auto& bauInfo = stationInfo.bauMap_[0];
    const auto& pcsInfo = stationInfo.pcsMap_[0];
    const auto& bauStatusSummary = bauInfo.bauStatusSummary;
    root["capacitySum"] = get_capacitySum(bauStatusSummary);
    root["env"] = get_env();
    root["mode"] = get_mode(stationInfo);
    root["meter"] = get_meter();
    root["storageSystem"] = get_storageSystem(bauStatusSummary);
    root["pvSystem"] = get_pvSystem();
    root["pvSystem"] = get_loadSystem();
    root["envManage"] = get_envManage();
    root["bms"] = get_bms(bauInfo);
    root["warning"] = get_warning(bauInfo, pcsInfo);
    root["peak"] = get_peak(bauStatusSummary);

    Json::Value repJson;
    repJson["data"] = root;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e){
    Json::Value msg;
    msg["data"] = Json::Value(Json::objectValue);
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

Json::Value MainWiringDiagram::get_realPower(const StationInfo& stationInfo)
{
    Json::Value content;
    auto& snapshot = stationInfo.powerRealtimeSnapshot;
    for(const auto& item: snapshot){
        Json::Value obj;
        obj["time"] = item.first;
        obj["value"] = stream_wrapper::serializeFloat(item.second, 1);
        content.append(obj);
    }
    return content;
}

Json::Value MainWiringDiagram::get_capacitySum(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    {
        auto& bms = content["bms"];
        bms["charge"] = stream_wrapper::serializeFloat(bauStatusSummary.chargeCapacitySum, 1);
        bms["discharge"] = stream_wrapper::serializeFloat(bauStatusSummary.dischargeCapacitySum, 1);
    }
    {
        auto& pv = content["pv"];
        pv["charge"] = "Reversed";
        pv["discharge"] = "Reversed";
    }
    return content;
}

Json::Value MainWiringDiagram::get_env()
{
    Json::Value content;
    content["air"] = "Reversed";
    content["tem"] = "Reversed";
    content["fire"] = "Reversed";
    content["hum"] = "Reversed";
    return content;
}

/**
 * 运行模式：
 *      如果安装了多个分支的情况，只要有一个分支并网成功，则显示并网，
 *      如果所有分支都处于离网状态，则显示为离网
 * 策略模式：
 *      如果只装配一个储能分支，则按实际执行情况显示；
 *      如果装配了多个分支，则显示为“混合”
*/
Json::Value MainWiringDiagram::get_mode(const StationInfo& stationInfo)
{
    Json::Value content;
    content["run"] = "并网";// 实际从PCS或STS设备获取，暂时没有数据;
    {
        auto& strategyInfo = stationInfo.strategyMap_.at(0);
        content["strategy"] = strategyInfo.autoRun ? strategyInfo.activeStrategy : "手动";
    }
    return content;
}

Json::Value MainWiringDiagram::get_meter()
{
    Json::Value content;
    content["volt"] = "Reserved";
    content["cur"] = "Reserved";
    content["power"] = "Reserved";
    return content;
}

Json::Value MainWiringDiagram::get_storageSystem(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;

    content["soc"] = to_string(bauStatusSummary.soc);
    content["status"] = "Reserved";
    content["volt"] = stream_wrapper::serializeFloat(bauStatusSummary.volt, 1);
    content["cur"] = stream_wrapper::serializeFloat(bauStatusSummary.cur, 2);
    {
        const double power{ bauStatusSummary.volt * bauStatusSummary.cur };
        const double powerKw{ power * 0.001 };
        content["power"] = stream_wrapper::serializeFloat(powerKw, 2);
    }
    return content;
}

Json::Value MainWiringDiagram::get_pvSystem()
{
    Json::Value content;
    content["status"] = "Reserved";
    content["volt"] = "Reserved";
    content["cur"] = "Reserved";
    content["power"] = "Reserved";
    content["soc"] = "Reserved";
    return content;
}

Json::Value MainWiringDiagram::get_loadSystem()
{
    Json::Value content;
    content["status"] = "Reserved";
    content["volt"] = "Reserved";
    content["cur"] = "Reserved";
    content["power"] = "Reserved";
    return content;
}

Json::Value MainWiringDiagram::get_envManage()
{
    Json::Value content;
    content["air"] = "Reserved";
    content["tem"] = "Reserved";
    content["fire"] = "Reserved";
    content["hum"] = "Reserved";
    return content;
}

Json::Value MainWiringDiagram::get_bms(const bau::BauInfo& bauInfo)
{
    Json::Value content;
    auto& bauStatusSummary = bauInfo.bauStatusSummary;

    content["runStatus"] = (bauInfo.runStatus.charge || bauInfo.runStatus.discharge) ? "运行" : "待机";
    content["gridStatus"] = (bauStatusSummary.bcuBingjiNum >0) ? "合并": "分离";
    content["soc"] = to_string(bauStatusSummary.soc);
    content["volt"] = stream_wrapper::serializeFloat(bauStatusSummary.volt, 2);
    content["cur"] = stream_wrapper::serializeFloat(bauStatusSummary.cur, 2);
    content["dischargeCapacity"] = stream_wrapper::serializeFloat(bauStatusSummary.dischargeCapacitySum, 2);
    return content;
}

Json::Value MainWiringDiagram::get_warning(const bau::BauInfo& bauInfo, const pcs::PcsInfo& pcsInfo)
{
    Json::Value content;
    {
        auto& obj = content["byDevice"];
        const int bmsWarningTotal = bauInfo.warningL1Count + bauInfo.warningL2Count + bauInfo.warningL3Count;
        obj["bms"] = to_string(bmsWarningTotal);

        const int pcsWarningTotal = pcsInfo.warningL1Count + pcsInfo.warningL2Count + pcsInfo.warningL3Count;
        obj["pcs"] = to_string(pcsWarningTotal);
    }
    {
        auto& obj = content["byGrade"];
        const int level1WarningTotal = bauInfo.warningL1Count + pcsInfo.warningL1Count;
        const int level2WarningTotal = bauInfo.warningL2Count + pcsInfo.warningL2Count;
        const int level3WarningTotal = bauInfo.warningL3Count + pcsInfo.warningL3Count;
        obj["level1"] = to_string(level1WarningTotal);
        obj["level2"] = to_string(level2WarningTotal);
        obj["level3"] = to_string(level3WarningTotal);
    }
    return content;
}

string MainWiringDiagram::convertCellAddrFormat(const uint16_t cellGlobalIndex)
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

Json::Value MainWiringDiagram::get_peak(const bau::BauStatusSummary& bauStatusSummary)
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
