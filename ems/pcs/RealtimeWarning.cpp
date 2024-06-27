#include "RealtimeWarning.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems::pcs;

RealtimeWarning::RealtimeWarning()
    : identity_("PcsRealtimeWarning")
    , dealer_(miscellaneous::createZmqSocket(zmq::socket_type::dealer))
{
    registerHttpInterfaces();
    dealer_.set(zmq::sockopt::routing_id, identity_);
    dealer_.connect("tcp://127.0.0.1:6200");
}

vector<byte> RealtimeWarning::identity()
{
    const auto beginItr = reinterpret_cast<const byte*>(identity_.data());
    return { beginItr, beginItr + identity_.size() };
}

void RealtimeWarning::registerHttpInterfaces()
{
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/pcsRealtimeWarning", httplib::Server::Handler(bind(&RealtimeWarning::requestCallback, this, _1, _2)));
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
    zmq::message_t sndmsg(serializedMsg.data(), serializedMsg.size());
    dealer_.send(zmq::message_t(), zmq::send_flags::sndmore);//empty
    dealer_.send(sndmsg, zmq::send_flags::none);
    // 接收消息
    zmq::message_t rcvmsg;
    (void)dealer_.recv(rcvmsg);

    // 先解析标准的响应消息
    pair<bool, vector<byte>> respondMsg;
    const bool unpackMsgResult = msgpackWrapper::unpack(rcvmsg.data(), rcvmsg.size(), respondMsg);
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

void RealtimeWarning::respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body)
{
try
{
    tuple<string> reqbody;
    const bool unpackResult = msgpackWrapper::unpack(body.data(), body.size(), reqbody);
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
    {
        const vector<byte> repcontent = miscellaneous::serializedJsonAsBytes(root);
        const pair<bool, vector<byte>> repbody{ true, repcontent };
        const msgpack::sbuffer repbodySerialized = msgpackWrapper::pack(repbody);

        zmq::message_t identitymsg(identity.data(), identity.size());
        zmq::message_t repmsg(repbodySerialized.data(), repbodySerialized.size());
        router.send(identitymsg, zmq::send_flags::sndmore);
        router.send(repmsg, zmq::send_flags::none);
    }
}
catch(const std::exception& e){
    const vector<byte> repcontent = miscellaneous::convertStringToBytes(e.what());
    const pair<bool, vector<byte>> repbody{ false, repcontent };
    const msgpack::sbuffer repbodySerialized = msgpackWrapper::pack(repbody);

    zmq::message_t identitymsg(identity.data(), identity.size());
    zmq::message_t repmsg(repbodySerialized.data(), repbodySerialized.size());
    router.send(identitymsg, zmq::send_flags::sndmore);
    router.send(repmsg, zmq::send_flags::none);
}
}
