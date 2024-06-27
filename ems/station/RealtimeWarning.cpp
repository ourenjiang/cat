#include "RealtimeWarning.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems;

RealtimeWarning::RealtimeWarning()
    : identity_("WarningDeviceTree")
    , dealer_(miscellaneous::createZmqSocket(zmq::socket_type::dealer))
{
    dealer_.set(zmq::sockopt::routing_id, identity_);
    dealer_.connect("tcp://127.0.0.1:6200");
    registerHttpInterfaces();
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
    serv.Get("/realtimeWarningDeviceTree", httplib::Server::Handler(bind(&RealtimeWarning::requestCallbackGetWarningDeviceTree, this, _1, _2)));
}

void RealtimeWarning::respondCallbackGetWarningDeviceTree(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body)
{
try
{
    Json::Value root(Json::arrayValue);
    for(const auto& item: stationInfo->branchList){

        Json::Value branchData;
        branchData["branchIndex"] = item.first;
        {
            auto& branchInfo = item.second;
            const auto& bauInfo = branchInfo->bauInfo;
            Json::Value bauData;
            bauData["bauActive"] = bauInfo.existActiveWarningOrFault ? "true": "false";

            for(const auto& bcu: bauInfo.bcuList){

                const auto& bcuInfo = bcu.second;
                Json::Value bcuData;
                bcuData["bcuIndex"] = to_string(bcu.first);
                bcuData["bcuActive"] = bcuInfo.existActiveWarningOrFault ? "true": "false";
                bauData["bcu"].append(bcuData);
            }
            branchData["bau"] = bauData;
        }
        {
            auto& branchInfo = item.second;
            const auto& pcsInfo = branchInfo->pcsInfo;
            branchData["pcsActive"] = pcsInfo.existActiveWarningOrFault ? "true": "false";
        }
        root.append(branchData);
    }

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

void RealtimeWarning::requestCallbackGetWarningDeviceTree(const httplib::Request &req, httplib::Response &res)
{
try
{
    dealer_.send(zmq::message_t(), zmq::send_flags::sndmore);
    dealer_.send(zmq::message_t(), zmq::send_flags::none);
    // 接收消息
    zmq::message_t rcvmsg;
    (void)dealer_.recv(rcvmsg);

    // 先解析标准的响应消息
    pair<bool, vector<byte>> respondMsg;
    const bool unpackMsgResult = msgpackWrapper::unpack(rcvmsg.data(), rcvmsg.size(), respondMsg);
    BOOST_ASSERT(unpackMsgResult);
    const auto& [returnStatus, returnContent] = respondMsg;
    if(!returnStatus){
        const string errmsg(reinterpret_cast<const char*>(returnContent.data()),
                                    reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());
        throw std::runtime_error(errmsg);
    }

    // 再解析自定义的响应内容
    const string jsonString(reinterpret_cast<const char*>(returnContent.data()),
                                reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());

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
