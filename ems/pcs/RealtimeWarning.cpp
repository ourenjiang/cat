#include "RealtimeWarning.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems::pcs;

RealtimeWarning::RealtimeWarning()
{
    registerHttpInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}
void RealtimeWarning::registerHttpInterfaces()
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/pcsWarningAndFault", httplib::Server::Handler(bind(&RealtimeWarning::requestCallback, this, _1, _2)));
}

void RealtimeWarning::requestCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("branchIndex"))
        throw std::runtime_error("request params err");
    tuple<string> workParams{ req.get_param_value("branchIndex") };

    // 准备请求参数
    auto serializedMsg = msgpackWrapper::pack(workParams);
    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("PcsRealtimeWarning");
    std::copy(topic.data(), topic.data() + topic.size(), std::back_inserter(publishContent));
    std::copy(serializedMsg.data(), serializedMsg.data() + serializedMsg.size(), std::back_inserter(publishContent));

    // 发送消息
    const vector<byte> sendmsg(reinterpret_cast<byte*>(publishContent.data()),
                                reinterpret_cast<byte*>(publishContent.data()) + publishContent.size());
    const bool sendResult = requester_->send(sendmsg);
    if(!sendResult) throw std::runtime_error("zmq send err");

    // 接收消息
    const auto recvResult = requester_->recv();
    if(!recvResult) throw std::runtime_error("zmq recv failed");
    const auto recvmsg = recvResult.value();

    // 先解析标准的响应消息
    pair<bool, vector<byte>> respondMsg;
    const bool unpackMsgResult = msgpackWrapper::unpack(recvmsg.data(), recvmsg.size(), respondMsg);
    BOOST_ASSERT(unpackMsgResult);
    const auto& [returnStatus, returnContent] = respondMsg;
    if(!returnStatus){
        // 再解析自定义的响应内容
        const string errmsg(reinterpret_cast<const char*>(returnContent.data()),
                                    reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());
        throw std::runtime_error(errmsg);
    }

    // 再解析自定义的响应内容
    const string respondContent(reinterpret_cast<const char*>(returnContent.data()),
                                reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());
    
    // 成功响应
    Json::Value repJson;
    repJson["data"] = miscellaneous::unserializedJson(respondContent);
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

vector<byte> RealtimeWarning::respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody)
{
    tuple<string> reqbody;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), reqbody);
    auto& [branchIndex] = reqbody;

    auto& branchList = stationInfo->branchList;
    auto branchItr = branchList.find(std::stoi(branchIndex));
    if(branchItr == branchList.end())
        throw std::runtime_error("invalid params");
    auto& branchInfo = branchItr->second;
    const auto& pcsInfo = branchInfo->pcsInfo;

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
    return miscellaneous::serializedJsonAsBytes(root);
}

string RealtimeWarning::getPcsWarningAndFault(std::shared_ptr<StationInfo> stationInfo, const int branchIndex)
{
    Json::Value content;
    auto& branchInfo = stationInfo->branchList[branchIndex];
    const auto& pcsInfo = branchInfo->pcsInfo;

    const auto& warningStatus = pcsInfo.warningStatus;
    Json::Value warningData;
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

    content["warning"] = warningData;
    content["warningL1Count"] = to_string(pcsInfo.warningL1Count);
    content["warningL2Count"] = to_string(pcsInfo.warningL2Count);
    content["warningL3Count"] = to_string(pcsInfo.warningL3Count);

    const auto& faultStatus = pcsInfo.faultStatus;
    Json::Value faultData;
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

    content["fault"] = faultData;
    content["faultCount"] = to_string(pcsInfo.faultCount);

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const string contentString = Json::writeString(builder, content);
    return contentString;
}
