#include "RealtimeWarning.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/base/StationInfo.h"

using namespace ems;
using namespace httplib;

RealtimeWarning::RealtimeWarning()
{
}

void RealtimeWarning::requestCallbackGetWarningDeviceTree(const Request &req, Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    auto stationInfo = base::getStationInfo(stationDealer);
    Json::Value root(Json::arrayValue);

    Json::Value branchData;
    branchData["branchIndex"] = "0";
    {
        const auto& bauInfo = stationInfo.bauMap_[0];
        Json::Value bauData;
        bauData["bauActive"] = bauInfo.existActiveWarningOrFault ? "true": "false";

        for(const auto& bcu: bauInfo.bcuList){

            const auto& bcuInfo = bcu.second;
            Json::Value bcuData;
            bcuData["bcuIndex"] = to_string(bcu.first);
            bcuData["bcuActive"] = bcuInfo.existActiveWarningOrFault ? "true": "false";
            bauData["bcu"].append(bcuData);
        }
        branchData["bau"] = bauData;
    }
    {
        const auto& pcsInfo = stationInfo.pcsMap_[0];
        branchData["pcsActive"] = pcsInfo.existActiveWarningOrFault ? "true": "false";
    }
    root.append(branchData);

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
