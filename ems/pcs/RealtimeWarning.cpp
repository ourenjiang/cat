#include "RealtimeWarning.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/base/StationInfo.h"

using namespace ems;
using namespace ems::pcs;

RealtimeWarning::RealtimeWarning()
{
}

void RealtimeWarning::requestCallback(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    if(!req.has_param("branchIndex"))
        throw std::runtime_error("request params err");
    const string branchIndex = req.get_param_value("branchIndex");
    ////////////////////////////////////////////////////////////////
    auto stationInfo = base::getStationInfo(stationDealer);

    // auto& branchList = stationInfo.branchList;
    // auto branchItr = branchList.find(std::stoi(branchIndex));
    // if(branchItr == branchList.end())
    //     throw std::runtime_error("invalid params");
    // auto& branchInfo = branchItr->second;
    const auto& pcsInfo = stationInfo.pcsMap_[std::stoi(branchIndex)];

    Json::Value root;
    root["warningL1Count"] = to_string(pcsInfo.warningL1Count);
    root["warningL2Count"] = to_string(pcsInfo.warningL2Count);
    root["warningL3Count"] = to_string(pcsInfo.warningL3Count);
    root["faultCount"] = to_string(pcsInfo.faultCount);

    const auto& warningStatus = pcsInfo.warningStatus;
    Json::Value& warningData = root["warning"];
    warningData["invertOverCur"] = to_string(warningStatus.warning1_invertOverCur);
    warningData["batteryVoltLow"] = to_string(warningStatus.warning1_batteryVoltLow);
    warningData["batteryChargeDisabled"] = to_string(warningStatus.warning1_batteryChargeDisabled);
    warningData["dcGeneratrixOverVolt"] = to_string(warningStatus.warning1_dcGeneratrixOverVolt);
    warningData["dcGeneratrixShortCircuit"] = to_string(warningStatus.warning1_dcGeneratrixShortCircuit);
    warningData["outputContactorOpenCircuit"] = to_string(warningStatus.warning1_outputContactorOpenCircuit);
    warningData["outputContactorShortCircuit"] = to_string(warningStatus.warning1_outputContactorShortCircuit);
    warningData["converterOverTem"] = to_string(warningStatus.warning1_converterOverTem);
    warningData["outputOverLoad"] = to_string(warningStatus.warning1_outputOverLoad);
    warningData["gridOverVolt"] = to_string(warningStatus.warning2_gridOverVolt);
    warningData["gridLackVolt"] = to_string(warningStatus.warning2_gridLackVolt);
    warningData["gridPhaseOrderReverse"] = to_string(warningStatus.warning2_gridPhaseOrderReverse);
    warningData["gridIslandingEffectProtect"] = to_string(warningStatus.warning2_gridIslandingEffectProtect);
    warningData["batteryDischargeDisabled"] = to_string(warningStatus.warning2_batteryDischargeDisabled);
    warningData["energentPowerOff"] = to_string(warningStatus.warning2_energentPowerOff);
    warningData["converterNotSync"] = to_string(warningStatus.warning2_converterNotSync);

    const auto& faultStatus = pcsInfo.faultStatus;
    Json::Value& faultData = root["fault"];
    faultData["zbLimitCurFault"] = to_string(faultStatus.warning1_zbLimitCurFault);
    faultData["converterFault"] = to_string(faultStatus.warning1_converterFault);
    faultData["bingjiCommFault"] = to_string(faultStatus.warning1_bingjiCommFault);
    faultData["batteryConnectReverse"] = to_string(faultStatus.warning1_batteryConnectReverse);
    faultData["dcContactorFault"] = to_string(faultStatus.warning1_dcContactorFault);
    faultData["bmsCommFault"] = to_string(faultStatus.warning1_bmsCommFault);
    faultData["inverterLackPhaseFault"] = to_string(faultStatus.warning1_inverterLackPhaseFault);
    faultData["gridFrequencyErr"] = to_string(faultStatus.warning2_gridFrequencyErr);
    faultData["drivingLineFault"] = to_string(faultStatus.warning2_drivingLineFault);
    faultData["lightningProtectFault"] = to_string(faultStatus.warning2_lightningProtectFault);
    faultData["insulationImpedanceErr"] = to_string(faultStatus.warning2_insulationImpedanceErr);
    faultData["invertOverVoltFault"] = to_string(faultStatus.warning2_invertOverVoltFault);
    faultData["15VPowerFault"] = to_string(faultStatus.warning2_15VPowerFault);
    faultData["acFanFault"] = to_string(faultStatus.warning2_acFanFault);
    faultData["batteryFault"] = to_string(faultStatus.warning2_batteryFault);
    faultData["ctOrHallOpenCircuitFault"] = to_string(faultStatus.warning2_ctOrHallOpenCircuitFault);
    
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
