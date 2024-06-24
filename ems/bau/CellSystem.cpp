#include "CellSystem.h"
#include "utils/YamlcppWrapper.h"
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems;
using namespace ems::bau;

CellSystem::CellSystem()
    : log_(ems::Log4cppWrapper::getLogger(5))
{
    registerAllInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void CellSystem::registerAllInterfaces()
{
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/cellSystem", bind(&CellSystem::requestCallback, this, _1, _2));
}

void CellSystem::requestCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("bauIndex")
        || !req.has_param("bcuIndex") || !req.has_param("bmuIndex")){
        throw std::runtime_error("request params err");
    }

    tuple<string, string, string> workParams{ req.get_param_value("bauIndex"),
                                            req.get_param_value("bcuIndex"), req.get_param_value("bmuIndex") };

    // 准备请求参数
    auto serializedMsg = msgpackWrapper::pack(workParams);

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("CellSystemGet");
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

vector<byte> CellSystem::respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    tuple<string, string, string> reqbody;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), reqbody);
    auto& [bauIndex, bcuIndex, bmuIndex] = reqbody;

    auto& branchList = stationInfo->branchList;
    auto branchItr = branchList.find(std::stoi(bauIndex));
    if(branchItr == branchList.end())
        throw std::invalid_argument("invalid params");

    const auto& branchInfo = branchItr->second;
    const auto& bauInfo = branchInfo->bauInfo;
    const auto& bcuList = bauInfo.bcuList;
    const auto& bcuItr = bcuList.find(std::stoi(bcuIndex));
    if(bcuItr == bcuList.end())
        throw std::invalid_argument("invalid params");

    const auto& bcuInfo = bcuItr->second;
    const auto& bmuList = bcuInfo.bmuList;
    const auto& bmuItr = bmuList.find(std::stoi(bmuIndex));
    if(bmuItr == bmuList.end()){
        throw std::invalid_argument("invalid params");
    }

    auto& bmuInfo = bmuItr->second;

    Json::Value root;
    root["statistic"] = bau::CellSystem::get_statistic(bmuInfo.cellvoltInfo, bmuInfo.celltemInfo);
    root["cellvolt"] = bau::CellSystem::get_cellvolt(bmuInfo.cellvoltInfo);
    root["celltem"] = bau::CellSystem::get_celltem(bmuInfo.celltemInfo);
    root["terminaltem"] = bau::CellSystem::get_terminaltem(bmuInfo.celltemInfo);

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const string jsonString = Json::writeString(builder, root);
    return { reinterpret_cast<const byte*>(jsonString.data()),
                reinterpret_cast<const byte*>(jsonString.data()) + jsonString.size() };
}

Json::Value CellSystem::get_statistic(const CellvoltSummary& cellvolt, const CelltemSummary& celltem)
{
    Json::Value root;

    {
        Json::Value obj;
        obj["value"] = miscellaneous::formatedPrecision(cellvolt.cellvoltMax.second, 2);
        obj["addr"] = to_string(cellvolt.cellvoltMax.first);
        root["voltMax"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = miscellaneous::formatedPrecision(cellvolt.cellvoltMin.second, 2);
        obj["addr"] = to_string(cellvolt.cellvoltMin.first);
        root["voltMin"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = miscellaneous::formatedPrecision(celltem.celltemMax.second, 1);
        obj["addr"] = to_string(celltem.celltemMax.first);
        root["temMax"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = miscellaneous::formatedPrecision(celltem.celltemMin.second, 1);
        obj["addr"] = to_string(celltem.celltemMin.first);
        root["temMin"] = obj;
    }
    root["voltDiff"] = miscellaneous::formatedPrecision(cellvolt.voltDiff, 2);
    root["temDiff"] = miscellaneous::formatedPrecision(celltem.temDiff, 1);
    return root;
}

Json::Value CellSystem::get_cellvolt(const CellvoltSummary& cellvolt)
{
    Json::Value content;
    for(const auto& item: cellvolt.cellvoltList){
        Json::Value obj;
        obj["key"] = to_string(item.first);
        obj["value"] = miscellaneous::formatedPrecision(item.second, 3);
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
        obj["value"] = miscellaneous::formatedPrecision(item.second, 1);
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
        obj["value"] = miscellaneous::formatedPrecision(item.second, 1);
        content.append(obj);
    }
    return content;
}
