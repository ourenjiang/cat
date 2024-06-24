#include "PublicInfo.h"
#include "utils/YamlcppWrapper.h"
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems;

PublicInfo::PublicInfo()
    : log_(ems::Log4cppWrapper::getLogger(5))
{
    registerAllInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
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
    
    tuple<string> workParams{ req.get_param_value("branchIndex") };

    // 准备请求参数
    auto serializedMsg = msgpackWrapper::pack(workParams);
    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("BranchPublicInfoGet");
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

vector<byte> PublicInfo::respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody)
{
    tuple<string> reqbody;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), reqbody);
    auto& [branchIndex] = reqbody;

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
    return miscellaneous::serializedJsonAsBytes(root);
}
