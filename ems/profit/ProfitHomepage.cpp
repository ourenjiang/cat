#include "ProfitHomepage.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/base/StationInfo.h"
#include "utils/StreamWrapper.h"

using namespace ems;

void Profit::requestCallback(const httplib::Request &req, httplib::Response &res,
    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    auto stationInfo = base::getStationInfo(dataDealer);

    Json::Value root;

    /**
     * 枚举值【策略模式】：
     * 101 - 两充两放
     * 102 - 三充三放
    */
    root["strategyMode"] = "Reserved";

    /**
     * 能量W(kWH) = 电量Q(AH) * 电压V(V) * 0.001
    */
    // const auto& StationInfo = storage_->getStationInfo();
    // const auto& bauInfo = StationInfo->branchList[0]->bauInfo;

    // const double chargeSum = bauInfo->bauStatusSummary->chargeCapacitySum;
    // const double dischargeSum = bauInfo->bauStatusSummary->dischargeCapacitySum;
    // const double volt = bauInfo->bauStatusSummary->volt;

    // const double chargeSumKwh = chargeSum * volt * 0.001;
    // const double dischargeSumKwh = dischargeSum * volt * 0.001;

    root["profitToday"] = "Reserved";//今日收益
    root["profitTotal"] = "Reserved";//累计收益

    const auto& bauStatusSummary = stationInfo.bauMap_[0].bauStatusSummary;
    root["soc"] = to_string(bauStatusSummary.soc);
    root["chargeCapacityToday"] = "Reserved";     //今日充电量
    root["dischargeCapacityToday"] = "Reserved";  //今日放电量
    root["chargeCapacityTotal"] = stream_wrapper::serializeFloat(bauStatusSummary.chargeCapacitySum, 2);//累计充电量
    root["dischargeCapacityTotal"] = stream_wrapper::serializeFloat(bauStatusSummary.dischargeCapacitySum, 2);//累计放电量
    root["bmsMode"] = "Reserved";
    root["allowChargeCapacity"] = stream_wrapper::serializeFloat(bauStatusSummary.allowChargeCapacity, 2);
    root["allowDischargeCapacity"] = stream_wrapper::serializeFloat(bauStatusSummary.allowDischargeCapacity, 2);
    root["deviceOnlineCounts"] = "Reserved";

    auto& snapshot = stationInfo.powerRealtimeSnapshot;
    for(const auto& item: snapshot){
        Json::Value obj;
        obj["time"] = item.first;
        obj["value"] = stream_wrapper::serializeFloat(item.second, 1);
        root["realPower"].append(obj);
    }

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
