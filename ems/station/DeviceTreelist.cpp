#include "DeviceTreeList.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/station/OperationRecord.h"
#include "utils/Miscellaneous.h"

using namespace ems;

DeviceTreeList::DeviceTreeList()
{
    registerHttpInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void DeviceTreeList::registerHttpInterfaces()
{
    using std::placeholders::_1;
    using std::placeholders::_2;
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

vector<byte> DeviceTreeList::respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg)
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

    // json序列化
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const string respondMessage = Json::writeString(builder, root);
    const vector<byte> sendmsg(reinterpret_cast<const byte*>(respondMessage.data()),
                                reinterpret_cast<const byte*>(respondMessage.data()) + respondMessage.size());
    return sendmsg;
}