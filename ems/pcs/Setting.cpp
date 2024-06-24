#include "Setting.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/station/OperationRecord.h"
#include "ems/station/UserManager.h"

using namespace ems;
using namespace ems::pcs;

Setting::Setting()
{
    registerHttpInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void Setting::registerHttpInterfaces()
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    auto& serv = utils::getHttpServerSingleton();
    serv.Post("/pcs/setting", httplib::Server::Handler(bind(&Setting::requestCallback, this, _1, _2)));
}

void Setting::requestCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    const auto reqbody = miscellaneous::unserializedJson(req.body);

    /* 鉴权 */
    Json::Value auth;
    if(!reqbody.isMember("auth"))
        throw std::runtime_error("request params err");
    auth = reqbody["auth"];
    if(!auth.isMember("username") || !auth.isMember("password"))
        throw std::runtime_error("request params err");
    if(!UserManager::doAuth(auth["username"].asString(), auth["password"].asString()))
        throw std::runtime_error("auth failed");

    /** 解析业务参数 */
    if(!reqbody.isMember("branchIndex") || !reqbody.isMember("type") || !reqbody.isMember("power"))
        throw std::runtime_error("request params err");
    tuple<string, string, string> workParams{ reqbody["branchIndex"].asString(),
                                        reqbody["type"].asString(), reqbody["power"].asString() };
    auto serializedMsg = msgpackWrapper::pack(workParams);

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("PcsSetting");
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

    // 保存'成功'操作记录
    const string status = "success";
    const string content{ "success" };
    const string type{ "参数设置" };
    const string timestamp = miscellaneous::getCurrentTimestamp();
    const string username = miscellaneous::getCurrentTimestamp();
    // OperationRecord::insertRecord(status, content, type, timestamp, username);

    // 成功响应
    Json::Value repJson;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e){
    // 保存'失败'操作记录
    const string status = "failed";
    const string content{ e.what() };
    const string type{ "参数设置" };
    const string timestamp = miscellaneous::getCurrentTimestamp();
    const string username = miscellaneous::getCurrentTimestamp();
    // OperationRecord::insertRecord(status, content, type, timestamp, username);

    // 失败响应
    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

vector<byte> Setting::respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    tuple<string, string, string> requestMsg;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), requestMsg);
    auto& [branchIndex, type, power] = requestMsg;

    // 如果数据操作成功，需要将更新同步到本地缓存
    // 由于本地缓存更新操作存在线程同步要求，因此这里将整个业务流封装为一个完整事务并在后台线程执行。


    // 返回结果
    return miscellaneous::convertStringToBytes("success");
}