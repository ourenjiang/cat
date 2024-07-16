#include "PublicInfo.h"
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/base/StationInfo.h"

using namespace ems;

void PublicInfo::requestCallback(const httplib::Request &req, httplib::Response &res,
                        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    if(!req.has_param("branchIndex"))
        throw std::runtime_error("request params err");
    const string branchIndex = req.get_param_value("branchIndex");

    auto stationInfo = base::getStationInfo(dataDealer);
    // auto& branchList = stationInfo.branchList;
    // auto branchItr = branchList.find(std::stoi(branchIndex));
    // if(branchItr == branchList.end())
    //     throw std::invalid_argument("invalid params");

    const auto& bauInfo = stationInfo.bauMap_[std::stoi(branchIndex)];

    Json::Value root;
    root["bauNum"] = to_string(stationInfo.bauMap_.size());
    root["pcsNum"] = to_string(stationInfo.bauMap_.size());
    root["bcuNum"] = to_string(bauInfo.bauStatusSummary.bcuNum);
    root["bmuNum"] = to_string(bauInfo.bauStatusSummary.bcuSize);
    root["version"] = "v1.0.1";

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
