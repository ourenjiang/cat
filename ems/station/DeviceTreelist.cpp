#include "DeviceTreeList.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/station/OperationRecord.h"
#include "utils/Miscellaneous.h"

using namespace ems;

DeviceTreeList::DeviceTreeList()
    : identity_("DeviceTreeList")
    , dealer_(miscellaneous::createZmqSocket(zmq::socket_type::dealer))
{
    registerHttpInterfaces();
    dealer_.set(zmq::sockopt::routing_id, identity_);
    dealer_.connect("tcp://127.0.0.1:6200");
}

void DeviceTreeList::registerHttpInterfaces()
{
    using namespace std::placeholders;
    using httplib::Server;
    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/deviceTreelist", bind(&DeviceTreeList::requestCallback, this, _1, _2));
}

void DeviceTreeList::requestCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("DeviceTreeListGet");
    std::copy(topic.data(), topic.data() + topic.size(), std::back_inserter(publishContent));

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

    // 最后根据实际数据结构进行解析
    const string jsonString(reinterpret_cast<const char*>(returnContent.data()),
                            reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());

    // 成功响应
    Json::Value repJson;
    repJson["data"] = miscellaneous::unserializedJson(jsonString);
    repJson["errcode"] = returnStatus ? 0 : -1;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e)
{
    // 失败响应
    Json::Value respondmsg;
    respondmsg["data"] = Json::Value(Json::objectValue);
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

vector<byte> DeviceTreeList::identity()
{
    const auto beginItr = reinterpret_cast<const byte*>(identity_.data());
    return { beginItr, beginItr + identity_.size() };
}

void DeviceTreeList::respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body)
{
try
{
    Json::Value root(Json::objectValue);
    for(const auto& branch: stationInfo->branchList){

        Json::Value branchData;
        branchData["branchNo"] = branch.first;
        
        const auto& branchInfo = branch.second;
        auto& bauInfo = branchInfo->bauInfo;
        branchData["bau"]["status"] = bauInfo.onlineFlag ? "online" : "offline";

        for(const auto& bcu: bauInfo.bcuList){

            Json::Value bcuData;
            bcuData["bcuNo"] = bcu.first;
            auto& bcuInfo = bcu.second;
            bcuData["status"] = bcuInfo.onlineFlag ? "online" : "offline";
            branchData["bau"]["bcu"].append(bcuData);
        }

        branchData["pcs"]["status"] = branchInfo->pcsInfo.onlineFlag ? "online" : "offline";
        root["branch"].append(branchData);
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