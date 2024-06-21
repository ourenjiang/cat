#include "RealtimeWarning.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems::bau;

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
    serv.Get("/bauWarningAndFault", httplib::Server::Handler(bind(&RealtimeWarning::requestCallbackBau, this, _1, _2)));
    serv.Get("/bcuWarningAndFault", httplib::Server::Handler(bind(&RealtimeWarning::requestCallbackBcu, this, _1, _2)));
}

void RealtimeWarning::requestCallbackBau(const httplib::Request &req, httplib::Response &res)
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
    const string topic = miscellaneous::createFixedSizeString("BauRealtimeWarning");
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

void RealtimeWarning::requestCallbackBcu(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("branchIndex") || !req.has_param("bcuIndex"))
        throw std::runtime_error("request params err");

    tuple<string, string> workParams{ req.get_param_value("branchIndex"), req.get_param_value("bcuIndex") };
    // 准备请求参数
    auto serializedMsg = msgpackWrapper::pack(workParams);
    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("BcuRealtimeWarning");
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

vector<byte> RealtimeWarning::respondCallbackBcu(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody)
{
    tuple<string, string> reqbody;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), reqbody);
    auto& [branchIndex, bcuIndex] = reqbody;

    auto& branchList = stationInfo->branchList;
    auto branchItr = branchList.find(std::stoi(branchIndex));
    if(branchItr == branchList.end())
        throw std::invalid_argument("invalid params");
    auto& branchInfo = branchItr->second;
    const auto& bauInfo = branchInfo->bauInfo;
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

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const string jsonString = Json::writeString(builder, root);
    return { reinterpret_cast<const byte*>(jsonString.data()),
                reinterpret_cast<const byte*>(jsonString.data()) + jsonString.size() };
}

vector<byte> RealtimeWarning::respondCallbackBau(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody)
{
    tuple<string> reqbody;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), reqbody);
    auto& [branchIndex] = reqbody;

    auto& branchList = stationInfo->branchList;
    auto branchItr = branchList.find(std::stoi(branchIndex));
    if(branchItr == branchList.end())
        throw std::invalid_argument("invalid params");
    auto& branchInfo = branchItr->second;
    const auto& bauInfo = branchInfo->bauInfo;

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

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const string jsonString = Json::writeString(builder, root);
    return { reinterpret_cast<const byte*>(jsonString.data()),
                reinterpret_cast<const byte*>(jsonString.data()) + jsonString.size() };
}
