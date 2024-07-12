#include "RealtimeWarning.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/base/StationInfo.h"

using namespace ems::bau;

RealtimeWarning::RealtimeWarning()
{
}

void RealtimeWarning::requestCallbackBau(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    if(!req.has_param("branchIndex"))
        throw std::runtime_error("request params err");
    const string branchIndex = req.get_param_value("branchIndex");
    //////////////////////////////////////////////////////////////////////////
    auto stationInfo = base::getStationInfo(stationDealer);

    // auto& branchList = stationInfo.branchList;
    // auto branchItr = branchList.find(std::stoi(branchIndex));
    // if(branchItr == branchList.end())
    //     throw std::invalid_argument("invalid params");
    // auto& branchInfo = branchItr->second;
    const auto& bauInfo = stationInfo.bauMap_[std::stoi(branchIndex)];

    Json::Value root;
    root["warningL1Count"] = to_string(bauInfo.warningL1Count);
    root["warningL2Count"] = to_string(bauInfo.warningL2Count);
    root["warningL3Count"] = to_string(bauInfo.warningL3Count);
    root["faultCount"] = to_string(bauInfo.faultCount);

    const auto& warningStatus = bauInfo.warningStatus;
    Json::Value& warningData = root["warning"];
    warningData["cellvoltHigh"] = to_string(warningStatus.cellVoltHigh);
    warningData["cellvoltLow"] = to_string(warningStatus.cellVoltLow);
    warningData["totalVoltHigh"] = to_string(warningStatus.totalVoltHigh);
    warningData["totalVoltLow"] = to_string(warningStatus.totalVoltLow);
    warningData["chargeOverCur"] = to_string(warningStatus.chargeOverCur);
    warningData["dischargeOverCur"] = to_string(warningStatus.dischargeOverCur);
    warningData["chargeTemHigh"] = to_string(warningStatus.chargeTemHigh);
    warningData["dischargeTemHigh"] = to_string(warningStatus.dischargeTemHigh);
    warningData["chargeTemLow"] = to_string(warningStatus.chargeTemLow);
    warningData["dischargeTemLow"] = to_string(warningStatus.dischargeTemLow);
    warningData["envTemHigh"] = to_string(warningStatus.envTemHigh);
    warningData["envTemLow"] = to_string(warningStatus.envTemLow);
    warningData["chargeRelayTemHigh"] = to_string(warningStatus.chargeRelayTemHigh);
    warningData["dischargeRelayTemHigh"] = to_string(warningStatus.dischargeRelayTemHigh);
    warningData["negativeRelayTemHigh"] = to_string(warningStatus.negativeRelayTemHigh);
    warningData["socHigh"] = to_string(warningStatus.socHigh);
    warningData["socLow"] = to_string(warningStatus.socLow);
    warningData["positiveInsulationLeakage"] = to_string(warningStatus.positiveInsulationLeakage);
    warningData["negativeInsulationLeakage"] = to_string(warningStatus.negativeInsulationLeakage);
    warningData["chargeVoltDiff"] = to_string(warningStatus.chargeVoltDiff);
    warningData["dischargeVoltDiff"] = to_string(warningStatus.dischargeVoltDiff);
    warningData["chargeTemDiff"] = to_string(warningStatus.chargeTemDiff);
    warningData["dischargeTemDiff"] = to_string(warningStatus.dischargeTemDiff);
    warningData["terminalTemHigh"] = to_string(warningStatus.terminalTemHigh);
    warningData["bcuVoltDiff"] = to_string(warningStatus.bcuVoltDiff);

    const auto& faultStatus = bauInfo.faultStatus;
    Json::Value& faultData = root["fault"];
    faultData["canBusErr"] = to_string(faultStatus.canBusErr);
    faultData["rs485Err"] = to_string(faultStatus.rs485Err);
    faultData["bcuVersionErr"] = to_string(faultStatus.bcuVersionErr);
    faultData["bcuAddrErr"] = to_string(faultStatus.bcuAddrErr);
    faultData["bcuOfflineErr"] = to_string(faultStatus.bcuOfflineErr);
    
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

void RealtimeWarning::requestCallbackBcu(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    if(!req.has_param("branchIndex") || !req.has_param("bcuIndex"))
        throw std::runtime_error("request params err");
    const string branchIndex = req.get_param_value("branchIndex");
    const string bcuIndex = req.get_param_value("bcuIndex");
    ////////////////////////////////////////////////////////////////////
    auto stationInfo = base::getStationInfo(stationDealer);

    // auto& branchList = stationInfo.branchList;
    // auto branchItr = branchList.find(std::stoi(branchIndex));
    // if(branchItr == branchList.end())
    //     throw std::invalid_argument("invalid params");
    // auto& branchInfo = branchItr->second;
    // const auto& bauInfo = branchInfo->bauInfo;
    const auto& bauInfo = stationInfo.bauMap_[std::stoi(branchIndex)];
    const auto& bcuList = bauInfo.bcuList;
    auto bcuItr = bcuList.find(std::stoi(bcuIndex));
    if(bcuItr == bcuList.end())
        throw std::invalid_argument("invalid params");
    const auto& bcuInfo = bcuItr->second;

    Json::Value root;
    root["warningL1Count"] = to_string(bcuInfo.warningL1Count);
    root["warningL2Count"] = to_string(bcuInfo.warningL2Count);
    root["warningL3Count"] = to_string(bcuInfo.warningL3Count);
    root["faultCount"] = to_string(bcuInfo.faultCount);

    const auto& warningStatus = bcuInfo.warningStatus;
    Json::Value& warningData = root["warning"];
    warningData["cellvoltHigh"] = to_string(warningStatus.cellVoltHigh);
    warningData["cellvoltLow"] = to_string(warningStatus.cellVoltLow);
    warningData["totalVoltHigh"] = to_string(warningStatus.totalVoltHigh);
    warningData["totalVoltLow"] = to_string(warningStatus.totalVoltLow);
    warningData["chargeOverCur"] = to_string(warningStatus.chargeOverCur);
    warningData["dischargeOverCur"] = to_string(warningStatus.dischargeOverCur);
    warningData["chargeTemHigh"] = to_string(warningStatus.chargeTemHigh);
    warningData["dischargeTemHigh"] = to_string(warningStatus.dischargeTemHigh);
    warningData["chargeTemLow"] = to_string(warningStatus.chargeTemLow);
    warningData["dischargeTemLow"] = to_string(warningStatus.dischargeTemLow);
    warningData["envTemHigh"] = to_string(warningStatus.envTemHigh);
    warningData["envTemLow"] = to_string(warningStatus.envTemLow);
    warningData["chargeRelayTemHigh"] = to_string(warningStatus.chargeRelayTemHigh);
    warningData["dischargeRelayTemHigh"] = to_string(warningStatus.dischargeRelayTemHigh);
    warningData["negativeRelayTemHigh"] = to_string(warningStatus.negativeRelayTemHigh);
    warningData["socHigh"] = to_string(warningStatus.socHigh);
    warningData["socLow"] = to_string(warningStatus.socLow);
    warningData["positiveInsulationLeakage"] = to_string(warningStatus.positiveInsulationLeakage);
    warningData["negativeInsulationLeakage"] = to_string(warningStatus.negativeInsulationLeakage);
    warningData["chargeVoltDiff"] = to_string(warningStatus.chargeVoltDiff);
    warningData["dischargeVoltDiff"] = to_string(warningStatus.dischargeVoltDiff);
    warningData["chargeTemDiff"] = to_string(warningStatus.chargeTemDiff);
    warningData["dischargeTemDiff"] = to_string(warningStatus.dischargeTemDiff);
    warningData["cellTemIncrease"] = to_string(warningStatus.cellTemIncrease);
    warningData["cellSampling"] = to_string(warningStatus.cellSampling);
    warningData["ntcSamplingErr"] = to_string(warningStatus.ntcSamplingErr);
    warningData["terminalTemHigh"] = to_string(warningStatus.terminalTemHigh);

    const auto& faultStatus = bcuInfo.faultStatus;
    Json::Value& faultData = root["fault"];
    faultData["chargeRelayCombine"] = to_string(faultStatus.chargeRelayCombine);
    faultData["chargeRelayDisabled"] = to_string(faultStatus.chargeRelayDisabled);
    faultData["dischargeRelayCombine"] = to_string(faultStatus.dischargeRelayCombine);
    faultData["dischargeRelayDisabled"] = to_string(faultStatus.dischargeRelayDisabled);
    faultData["prechargeRelayCombine"] = to_string(faultStatus.prechargeRelayCombine);
    faultData["prechargeRelayDisabled"] = to_string(faultStatus.prechargeRelayDisabled);
    faultData["negativeRelayCombine"] = to_string(faultStatus.negativeRelayCombine);
    faultData["negativeRelayDisabled"] = to_string(faultStatus.negativeRelayDisabled);
    faultData["heatingFilmRelayCombine"] = to_string(faultStatus.heatingFilmRelayCombine);
    faultData["heatingFileRelayDisabled"] = to_string(faultStatus.heatingFileRelayDisabled);
    faultData["_12VErr"] = to_string(faultStatus._12VErr);
    faultData["cellFault"] = to_string(faultStatus.cellFault);
    faultData["prechargeFault"] = to_string(faultStatus.prechargeFault);
    faultData["heatingFilmFault"] = to_string(faultStatus.heatingFilmFault);
    faultData["insulationBoardCommFault"] = to_string(faultStatus.insulationBoardCommFault);
    faultData["samplingBoardCommFault"] = to_string(faultStatus.samplingBoardCommFault);
    faultData["curDiverterFault"] = to_string(faultStatus.curDiverterFault);
    faultData["ntcFault"] = to_string(faultStatus.ntcFault);

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
