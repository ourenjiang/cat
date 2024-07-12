#include "HeapSystem.h"
#include "boost/assert.hpp"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/base/StationInfo.h"
#include "utils/StreamWrapper.h"

using namespace ems;
using namespace ems::bau;

void HeapSystem::requestCallback(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    if(!req.has_param("branchIndex"))
        throw std::runtime_error("request params err");
    const string branchIndex = req.get_param_value("branchIndex");
    
    auto stationInfo = base::getStationInfo(stationDealer);
    // auto& branchList = stationInfo.branchList;
    // auto branchItr = branchList.find(std::stoi(branchIndex));
    // if(branchItr == branchList.end())
    //     throw std::invalid_argument("invalid params");

    // const auto& branchInfo = branchItr->second;
    // const auto& bauInfo = branchInfo->bauInfo;
    const auto& bauInfo = stationInfo.bauMap_[std::stoi(branchIndex)];

    Json::Value root;
    root["base"] = get_base(bauInfo.bauStatusSummary);
    root["powerEnvSystem"] = get_powerEnvSystem();
    root["branch"] = get_bcu(bauInfo.bcuList);
    {
        auto& content = root["warning"];
        content["level1"] = to_string(bauInfo.warningL1Count);
        content["level2"] = to_string(bauInfo.warningL2Count);
        content["level3"] = to_string(bauInfo.warningL3Count);
    }
    root["peak"] = get_peak(bauInfo.bauStatusSummary);

    // 成功响应
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

Json::Value HeapSystem::get_base(const BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    content["bingjiNum"] = to_string(bauStatusSummary.bcuBingjiNum);
    content["soc"] = to_string(bauStatusSummary.soc);
    content["soh"] = to_string(bauStatusSummary.soh);
    content["allowChargeCapacity"] = stream_wrapper::serializeFloat(bauStatusSummary.allowChargeCapacity, 2);
    content["allowDischargeCapacity"] = stream_wrapper::serializeFloat(bauStatusSummary.allowDischargeCapacity, 2);
    content["volt"] = stream_wrapper::serializeFloat(bauStatusSummary.volt, 2);
    content["cur"] = stream_wrapper::serializeFloat(bauStatusSummary.cur, 2);
    content["clusterNum"] = to_string(bauStatusSummary.bcuNum);
    content["onlineNum"] = to_string(bauStatusSummary.bcuOnlineNum);
    content["cellNum"] = to_string(bauStatusSummary.bcuNum
                                    * bauStatusSummary.bcuSize
                                    * bauStatusSummary.bmuCellVoltSize
                                    );
    return content;
}

Json::Value HeapSystem::get_powerEnvSystem()
{
    Json::Value content;
    content["key"] = "Reserved";
    return content;
}

Json::Value HeapSystem::get_bcu(const map<int, BcuInfo>& bcuMap)
{
    Json::Value content(Json::arrayValue);

    for(const auto& item: bcuMap){
        Json::Value info = get_bcu(item.second);
        info["index"] = to_string(item.first);
        content.append(info);
    }
    return content;
}

Json::Value HeapSystem::get_bcu(const BcuInfo& bcuInfo)
{
    Json::Value content;

    const auto& bcuStatus{ bcuInfo.base };
    {
        // 并网 | 离网
        auto& relayRealStatus = bcuInfo.relayRealStatus;
        const bool isGrid = relayRealStatus.chargeRelay
            || relayRealStatus.dischargeRelay || relayRealStatus.negativeRelay;
        content["status"] = isGrid ? "并网" : "离网";
    }
    content["volt"] = stream_wrapper::serializeFloat(bcuStatus.volt, 2);
    content["cur"] = stream_wrapper::serializeFloat(bcuStatus.cur, 2);
    {
        const double power{ bcuStatus.volt * bcuStatus.cur };
        const double powerKw{ power * 0.001 };
        content["power"] = stream_wrapper::serializeFloat(powerKw, 2);
    }
    content["soc"] = to_string(bcuStatus.soc);
    content["soh"] = to_string(bcuStatus.soh);
    content["cellvoltMax"] = stream_wrapper::serializeFloat(bcuStatus.cellvoltMax, 3);
    content["cellvoltMin"] = stream_wrapper::serializeFloat(bcuStatus.cellvoltMin, 3);
    content["celltemMax"] = stream_wrapper::serializeFloat(bcuStatus.celltemMax, 1);
    content["celltemMin"] = stream_wrapper::serializeFloat(bcuStatus.celltemMin, 1);
    return content;
}

string HeapSystem::convertCellAddrFormat(const uint16_t cellGlobalIndex)
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

Json::Value HeapSystem::get_peak(const BauStatusSummary& bauStatusSummary)
{
    Json::Value content;

    {
        Json::Value obj;
        obj["value"] = stream_wrapper::serializeFloat(bauStatusSummary.cellvoltMax, 3);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.cellvoltMaxAddr);
        content["cellvoltMax"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = stream_wrapper::serializeFloat(bauStatusSummary.cellvoltMin, 3);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.cellvoltMinAddr);
        content["cellvoltMin"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = stream_wrapper::serializeFloat(bauStatusSummary.celltemMax, 1);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.celltemMaxAddr);
        content["celltemMax"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = stream_wrapper::serializeFloat(bauStatusSummary.celltemMin, 1);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.celltemMinAddr);
        content["celltemMin"] = obj;
    }
    return content;
}
