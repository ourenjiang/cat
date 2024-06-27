#include "PublicInfo.h"
#include "utils/YamlcppWrapper.h"
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems;

PublicInfo::PublicInfo()
    : log_(ems::Log4cppWrapper::getLogger(5))
    , identity_("BranchPublicInfoGet")
    , dealer_(miscellaneous::createZmqSocket(zmq::socket_type::dealer))
{
    registerAllInterfaces();
    dealer_.set(zmq::sockopt::routing_id, identity_);
    dealer_.connect("tcp://127.0.0.1:6200");
}

vector<byte> PublicInfo::dealerIdentity()
{
    const auto beginItr = reinterpret_cast<const byte*>(identity_.data());
    return { beginItr, beginItr + identity_.size() };
}

void PublicInfo::registerAllInterfaces()
{
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/branchPublicInfo", bind(&PublicInfo::requestCallback, this, _1, _2));
}

void PublicInfo::requestCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("branchIndex"))
        throw std::runtime_error("request params err");

    string workParams = req.get_param_value("branchIndex");
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

void PublicInfo::respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body)
{
try
{
    string reqbody;
    const bool unpackResult = msgpackWrapper::unpack(body.data(), body.size(), reqbody);
    const string& branchIndex = reqbody;

    auto& branchList = stationInfo->branchList;
    auto branchItr = branchList.find(std::stoi(branchIndex));
    if(branchItr == branchList.end())
        throw std::invalid_argument("invalid params");

    const auto& branchInfo = branchItr->second;
    const auto& bauInfo = branchInfo->bauInfo;

    Json::Value root;
    root["bauNum"] = to_string(branchList.size());
    root["pcsNum"] = to_string(branchList.size());
    root["bcuNum"] = to_string(bauInfo.bauStatusSummary.bcuNum);
    root["bmuNum"] = to_string(bauInfo.bauStatusSummary.bcuSize);

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
