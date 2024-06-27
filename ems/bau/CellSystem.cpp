#include "CellSystem.h"
#include "utils/YamlcppWrapper.h"
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems;
using namespace ems::bau;

CellSystem::CellSystem()
    : log_(ems::Log4cppWrapper::getLogger(5))
    , identity_("CellSystemGet")
    , dealer_(miscellaneous::createZmqSocket(zmq::socket_type::dealer))
{
    registerAllInterfaces();
    dealer_.set(zmq::sockopt::routing_id, identity_);
    dealer_.connect("tcp://127.0.0.1:6200");
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
    if(!req.has_param("branchIndex")
        || !req.has_param("bcuIndex") || !req.has_param("bmuIndex")){
        throw std::runtime_error("request params err");
    }

    tuple<string, string, string> workParams{ req.get_param_value("branchIndex"),
                                            req.get_param_value("bcuIndex"), req.get_param_value("bmuIndex") };

    // 准备请求参数
    auto serializedMsg = msgpackWrapper::pack(workParams);
    zmq::message_t sndmsg(serializedMsg.data(), serializedMsg.size());
    dealer_.send(zmq::message_t(), zmq::send_flags::sndmore);
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
    const string jsonString(reinterpret_cast<const char*>(returnContent.data()),
                                reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());

    // 成功响应
    Json::Value repJson;
    repJson["data"] = miscellaneous::unserializedJson(jsonString);
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

vector<byte> CellSystem::identity()
{
    const auto beginItr = reinterpret_cast<const byte*>(identity_.data());
    return { beginItr, beginItr + identity_.size() };
}

void CellSystem::respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body) const
{
try
{
    tuple<string, string, string> reqbody;
    const bool unpackResult = msgpackWrapper::unpack(body.data(), body.size(), reqbody);
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
