#include "DeviceTreeList.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/interface/OperationRecord.h"
#include "utils/Miscellaneous.h"
#include "ems/base/StationInfo.h"

using namespace ems;

DeviceTreeList::DeviceTreeList()
{
}

void DeviceTreeList::requestCallback(const httplib::Request &req, httplib::Response &res,
                        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    auto stationInfo = base::getStationInfo(dataDealer);
    Json::Value root(Json::objectValue);

    Json::Value branchData;
    branchData["branchNo"] = "0";
    
    // const auto& branchInfo = branch.second;
    auto& bauInfo = stationInfo.bauMap_[0];
    // branchData["bau"]["status"] = bauInfo.onlineFlag ? "online" : "offline";
    branchData["bau"]["status"] = "offline";

    for(const auto& bcu: bauInfo.bcuList){

        Json::Value bcuData;
        bcuData["bcuNo"] = bcu.first;
        auto& bcuInfo = bcu.second;
        // bcuData["status"] = bcuInfo.onlineFlag ? "online" : "offline";
        bcuData["status"] = "offline";
        branchData["bau"]["bcu"].append(bcuData);
    }

    branchData["pcs"]["status"] = "offline";
    root["branch"].append(branchData);

    // 成功响应
    Json::Value repJson;
    repJson["data"] = root;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e)
{
    // 失败响应
    Json::Value respondmsg;
    respondmsg["data"] = Json::Value(Json::objectValue);
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}
