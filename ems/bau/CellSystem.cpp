#include "CellSystem.h"
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/base/StationInfo.h"
#include "utils/StreamWrapper.h"

using namespace ems;
using namespace ems::bau;

void CellSystem::requestCallback(const httplib::Request &req, httplib::Response &res,
                                shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    if(!req.has_param("branchIndex")
        || !req.has_param("bcuIndex") || !req.has_param("bmuIndex"))
        throw std::runtime_error("request params err");
    const string branchIndex = req.get_param_value("branchIndex");
    const string bcuIndex = req.get_param_value("bcuIndex");
    const string bmuIndex = req.get_param_value("bmuIndex");
    ////////////////////////////////////////////////////////////////

    auto stationInfo = base::getStationInfo(stationDealer);
    // auto& branchList = stationInfo.branchList;
    // auto branchItr = branchList.find(std::stoi(branchIndex));
    // if(branchItr == branchList.end())
    //     throw std::invalid_argument("invalid params");

    // const auto& branchInfo = branchItr->second;
    const auto& bauInfo = stationInfo.bauMap_[std::stoi(branchIndex)];
    const auto& bcuList = bauInfo.bcuList;
    const auto& bcuItr = bcuList.find(std::stoi(bcuIndex));
    if(bcuItr == bcuList.end())
        throw std::invalid_argument("invalid params");

    const auto& bcuInfo = bcuItr->second;
    const auto& bmuList = bcuInfo.bmuList;
    const auto& bmuItr = bmuList.find(std::stoi(bmuIndex));
    if(bmuItr == bmuList.end())
        throw std::invalid_argument("invalid params");

    auto& bmuInfo = bmuItr->second;

    Json::Value root;
    root["statistic"] = bau::CellSystem::get_statistic(bmuInfo.cellvoltInfo, bmuInfo.celltemInfo);
    root["cellvolt"] = bau::CellSystem::get_cellvolt(bmuInfo.cellvoltInfo);
    root["celltem"] = bau::CellSystem::get_celltem(bmuInfo.celltemInfo);
    root["terminaltem"] = bau::CellSystem::get_terminaltem(bmuInfo.celltemInfo);

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

Json::Value CellSystem::get_statistic(const CellvoltSummary& cellvolt, const CelltemSummary& celltem)
{
    Json::Value root;

    {
        Json::Value obj;
        obj["value"] = stream_wrapper::serializeFloat(cellvolt.cellvoltMax.second, 3);
        obj["addr"] = to_string(cellvolt.cellvoltMax.first);
        root["voltMax"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = stream_wrapper::serializeFloat(cellvolt.cellvoltMin.second, 3);
        obj["addr"] = to_string(cellvolt.cellvoltMin.first);
        root["voltMin"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = stream_wrapper::serializeFloat(celltem.celltemMax.second, 1);
        obj["addr"] = to_string(celltem.celltemMax.first);
        root["temMax"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = stream_wrapper::serializeFloat(celltem.celltemMin.second, 1);
        obj["addr"] = to_string(celltem.celltemMin.first);
        root["temMin"] = obj;
    }
    root["voltDiff"] = stream_wrapper::serializeFloat(cellvolt.voltDiff, 3);
    root["temDiff"] = stream_wrapper::serializeFloat(celltem.temDiff, 1);
    return root;
}

Json::Value CellSystem::get_cellvolt(const CellvoltSummary& cellvolt)
{
    Json::Value content;
    for(const auto& item: cellvolt.cellvoltList){
        Json::Value obj;
        obj["key"] = to_string(item.first);
        obj["value"] = stream_wrapper::serializeFloat(item.second, 3);
        content.append(obj);
    }
    return content;
}

Json::Value CellSystem::get_celltem(const CelltemSummary& celltem)
{
    Json::Value content;
    for(const auto& item: celltem.cellTemList){
        Json::Value obj;
        obj["key"] = to_string(item.first);
        obj["value"] = stream_wrapper::serializeFloat(item.second, 1);
        content.append(obj);
    }
    return content;
}

Json::Value CellSystem::get_terminaltem(const CelltemSummary& celltem)
{
    Json::Value content;
    for(const auto& item: celltem.terminalTemList){
        Json::Value obj;
        obj["key"] = to_string(item.first);
        obj["value"] = stream_wrapper::serializeFloat(item.second, 1);
        content.append(obj);
    }
    return content;
}
