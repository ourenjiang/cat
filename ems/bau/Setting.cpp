#include "Setting.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/station/OperationRecord.h"
#include "ems/station/UserManager.h"

using namespace ems;
using namespace ems::bau;

Setting::Setting()
    : zmqDealerBau_(zmqContext_, zmq::socket_type::dealer)
{
    registerHttpInterfaces();
    stationRequester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");

    {
        auto& cfgRoot = YamlcppWrapper::getRoot();
        const auto& collectors = cfgRoot["collector"];
        auto resultLoad = std::find_if(collectors.begin(), collectors.end(), [](const YAML::Node& item){
            return item["name"].as<string>() == "BAU";
        });
        BOOST_ASSERT(resultLoad != collectors.end());
        BOOST_ASSERT((*resultLoad)["load"].as<bool>());

        const string proxyAddress = (*resultLoad)["master"]["address"].as<string>();
        // bauRequester_ = make_shared<ZmqRequest>(proxyAddress);
        zmqDealerBau_.connect(proxyAddress);
    }
}

void Setting::registerHttpInterfaces()
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    auto& serv = utils::getHttpServerSingleton();
    serv.Post("/bau/powerOff", httplib::Server::Handler(bind(&Setting::requestCallbackPowerOff, this, _1, _2)));
    serv.Post("/bau/quickStartup", httplib::Server::Handler(bind(&Setting::requestCallbackQuickStartup, this, _1, _2)));
    serv.Post("/bau/setBcuRelay", httplib::Server::Handler(bind(&Setting::requestCallbackSetBcuRelay, this, _1, _2)));
}

void Setting::requestCallbackPowerOff(const httplib::Request &req, httplib::Response &res)
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
    const string usernameAuth = auth["username"].asString();
    const string passwordAuth = auth["password"].asString();
    if(!UserManager::doAuth(usernameAuth, passwordAuth))
        throw std::runtime_error("auth failed");

    /** 解析业务参数 */
    if(!reqbody.isMember("branchIndex"))
        throw std::runtime_error("request params err");
    string workParams{ reqbody["branchIndex"].asString() };
    auto serializedMsg = msgpackWrapper::pack(workParams);
    
    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("BauSettingPowerOff");
    std::copy(topic.data(), topic.data() + topic.size(), std::back_inserter(publishContent));
    std::copy(serializedMsg.data(), serializedMsg.data() + serializedMsg.size(), std::back_inserter(publishContent));

    // 发送消息
    const vector<byte> sendmsg(reinterpret_cast<byte*>(publishContent.data()),
                                reinterpret_cast<byte*>(publishContent.data()) + publishContent.size());
    const bool sendResult = stationRequester_->send(sendmsg);
    if(!sendResult) throw std::runtime_error("zmq send err");

    // 接收消息
    const auto recvResult = stationRequester_->recv();
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
    OperationRecord::insertRecord("success", "BAU关机", "参数设置", usernameAuth);

    // 成功响应
    Json::Value repJson;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e){
    
    // 保存'失败'操作记录
    OperationRecord::insertRecord("failed", e.what(), "参数设置", "username");

    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

void Setting::requestCallbackQuickStartup(const httplib::Request &req, httplib::Response &res)
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
    const string usernameAuth = auth["username"].asString();
    const string passwordAuth = auth["password"].asString();
    if(!UserManager::doAuth(usernameAuth, passwordAuth))
        throw std::runtime_error("auth failed");

    /** 解析业务参数 */
    if(!reqbody.isMember("branchIndex"))
        throw std::runtime_error("request params err");
    string workParams{ reqbody["branchIndex"].asString() };
    auto serializedMsg = msgpackWrapper::pack(workParams);
    
    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("BauSettingQuickStartup");
    std::copy(topic.data(), topic.data() + topic.size(), std::back_inserter(publishContent));
    std::copy(serializedMsg.data(), serializedMsg.data() + serializedMsg.size(), std::back_inserter(publishContent));

    // 发送消息
    const vector<byte> sendmsg(reinterpret_cast<byte*>(publishContent.data()),
                                reinterpret_cast<byte*>(publishContent.data()) + publishContent.size());
    const bool sendResult = stationRequester_->send(sendmsg);
    if(!sendResult) throw std::runtime_error("zmq send err");

    // 接收消息
    const auto recvResult = stationRequester_->recv();
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
    OperationRecord::insertRecord("success", "success", "参数设置", usernameAuth);

    // 成功响应
    Json::Value repJson;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e){
    const string status = "failed";
    const string content{ e.what() };
    const string type{ "参数设置" };
    const string timestamp = miscellaneous::getCurrentTimestamp();
    const string username = miscellaneous::getCurrentTimestamp();
    // OperationRecord::insertRecord(status, content, type, timestamp, username);

    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

void Setting::requestCallbackSetBcuRelay(const httplib::Request &req, httplib::Response &res)
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
    if(!reqbody.isMember("branchIndex")
        || !reqbody.isMember("bcuIndex") || !reqbody.isMember("status") )
        throw std::runtime_error("request params err");
    const string branchIndex = reqbody["branchIndex"].asString();
    const string bcuIndex = reqbody["bcuIndex"].asString();
    const string status = reqbody["status"].asString();
    if(status != "on" && status != "off")
        throw std::runtime_error("request params err");
    tuple<string, string, string> workParams{ branchIndex, bcuIndex, status };
    auto serializedMsg = msgpackWrapper::pack(workParams);

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("BauSettingSetBcuRelay");
    std::copy(topic.data(), topic.data() + topic.size(), std::back_inserter(publishContent));
    std::copy(serializedMsg.data(), serializedMsg.data() + serializedMsg.size(), std::back_inserter(publishContent));

    // 发送消息
    const vector<byte> sendmsg(reinterpret_cast<byte*>(publishContent.data()),
                                reinterpret_cast<byte*>(publishContent.data()) + publishContent.size());
    const bool sendResult = stationRequester_->send(sendmsg);
    if(!sendResult) throw std::runtime_error("zmq send err");

    // 接收消息
    const auto recvResult = stationRequester_->recv();
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
    // const string status = "success";
    // const string content{ "success" };
    // const string type{ "参数设置" };
    // const string timestamp = miscellaneous::getCurrentTimestamp();
    // const string username = miscellaneous::getCurrentTimestamp();
    // OperationRecord::insertRecord(status, content, type, timestamp, username);

    // 成功响应
    Json::Value repJson;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e){
    const string status = "failed";
    const string content{ e.what() };
    const string type{ "参数设置" };
    const string timestamp = miscellaneous::getCurrentTimestamp();
    const string username = miscellaneous::getCurrentTimestamp();
    // OperationRecord::insertRecord(status, content, type, timestamp, username);

    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

vector<byte> Setting::respondCallbacPowerOff(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody)
{
    string requestMsg;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), requestMsg);
    const string branchIndex = requestMsg;

    // 先检查分支、簇的状态
    auto& branchList = stationInfo->branchList;
    auto branchItr = branchList.find(std::stoi(branchIndex));
    if(branchItr == branchList.end()) throw std::runtime_error("branch not exist");

    // 准备Modbus请求帧:
    // 0xd700  掉电关机   固定0
    // 0xd701  一键并机   固定0
    // 0xd702  簇分离     簇下标从1开始，0代表所有
    // 0xd703  簇合并     簇下标从1开始，0代表所有
    uint16_t regAddress{ 0xd700 };
    uint16_t regData{ 0 };
    auto reqmsg = miscellaneous::createModbusRtuWriteFrame(0x01, 0x06, regAddress, regData);

    zmq::message_t sndmsg(reqmsg.data(), reqmsg.size());
    zmqDealerBau_.send(sndmsg, zmq::send_flags::none);

    zmq::pollitem_t item{ zmqDealerBau_, 0, ZMQ_POLLIN, 0 };
    const int pollResult = zmq::poll(&item, 1, std::chrono::seconds(1));
    BOOST_ASSERT(pollResult == 0 || pollResult == 1);
    if(pollResult == 0)
        throw std::runtime_error("zmq recv failed");

    zmq::message_t rcvmsg;
    (void)zmqDealerBau_.recv(rcvmsg);
    const vector<uint8_t> repmsg(reinterpret_cast<uint8_t*>(rcvmsg.data()),
                            reinterpret_cast<uint8_t*>(rcvmsg.data()) + rcvmsg.size());
    if(reqmsg != repmsg)
        throw std::runtime_error("recv frame err");

    // 返回结果
    const string result{ "success" };
    return { reinterpret_cast<const byte*>(result.data()),
            reinterpret_cast<const byte*>(result.data()) + result.size() };
}

vector<byte> Setting::respondCallbacQuickStartup(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody)
{
    string requestMsg;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), requestMsg);
    const string branchIndex = requestMsg;
    
    // 先检查分支、簇的状态
    auto& branchList = stationInfo->branchList;
    auto branchItr = branchList.find(std::stoi(branchIndex));
    if(branchItr == branchList.end()) throw std::runtime_error("branch not exist");

    // 准备Modbus请求帧:
    // 0xd700  掉电关机   固定0
    // 0xd701  一键并机   固定0
    // 0xd702  簇分离     簇下标从1开始，0代表所有
    // 0xd703  簇合并     簇下标从1开始，0代表所有
    uint16_t regAddress{ 0xd701 };
    uint16_t regData{ 0 };
    auto reqmsg = miscellaneous::createModbusRtuWriteFrame(0x01, 0x06, regAddress, regData);

    zmq::message_t sndmsg(reqmsg.data(), reqmsg.size());
    zmqDealerBau_.send(sndmsg, zmq::send_flags::none);

    zmq::pollitem_t item{ zmqDealerBau_, 0, ZMQ_POLLIN, 0 };
    const int pollResult = zmq::poll(&item, 1, std::chrono::seconds(1));
    BOOST_ASSERT(pollResult == 0 || pollResult == 1);
    if(pollResult == 0)
        throw std::runtime_error("zmq recv failed");

    zmq::message_t rcvmsg;
    (void)zmqDealerBau_.recv(rcvmsg);
    const vector<uint8_t> repmsg(reinterpret_cast<uint8_t*>(rcvmsg.data()),
                            reinterpret_cast<uint8_t*>(rcvmsg.data()) + rcvmsg.size());
    if(reqmsg != repmsg)
        throw std::runtime_error("recv frame err");

    // 返回结果
    const string result{ "success" };
    return { reinterpret_cast<const byte*>(result.data()),
            reinterpret_cast<const byte*>(result.data()) + result.size() };
}

vector<byte> Setting::respondCallbacSetBcuRelay(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody)
{
    tuple<string, string, string> requestMsg;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), requestMsg);
    auto& [branchIndex, bcuIndex, status] = requestMsg;

    // 先检查分支、簇的状态
    auto& branchList = stationInfo->branchList;
    auto branchItr = branchList.find(std::stoi(branchIndex));
    if(branchItr == branchList.end()) throw std::runtime_error("branch not exist");
    auto& branchInfo = branchItr->second;
    auto& bauInfo = branchInfo->bauInfo;
    auto& bcuList = bauInfo.bcuList;
    auto bcuItr = bcuList.find(std::stoi(bcuIndex));
    if(bcuItr == bcuList.end()) throw std::runtime_error("bcu not exist");

    // 准备Modbus请求帧:
    // 0xd701  一键并机   固定0
    // 0xd702  簇分离     簇下标从1开始，0代表所有
    // 0xd703  簇合并     簇下标从1开始，0代表所有
    uint16_t regAddress{ 0 };
    if(status == "on") regAddress = 0xd703;
    if(status == "off") regAddress = 0xd702;
    uint16_t regData{ 0 };
    auto reqmsg = miscellaneous::createModbusRtuWriteFrame(0x01, 0x06, regAddress, regData);

    zmq::message_t sndmsg(reqmsg.data(), reqmsg.size());
    zmqDealerBau_.send(sndmsg, zmq::send_flags::none);

    zmq::pollitem_t item{ zmqDealerBau_, 0, ZMQ_POLLIN, 0 };
    const int pollResult = zmq::poll(&item, 1, std::chrono::seconds(1));
    BOOST_ASSERT(pollResult == 0 || pollResult == 1);
    if(pollResult == 0)
        throw std::runtime_error("zmq recv failed");

    zmq::message_t rcvmsg;
    (void)zmqDealerBau_.recv(rcvmsg);
    const vector<uint8_t> repmsg(reinterpret_cast<uint8_t*>(rcvmsg.data()),
                            reinterpret_cast<uint8_t*>(rcvmsg.data()) + rcvmsg.size());
    if(reqmsg != repmsg)
        throw std::runtime_error("recv frame err");

    // 返回结果
    const string result{ "success" };
    return { reinterpret_cast<const byte*>(result.data()),
            reinterpret_cast<const byte*>(result.data()) + result.size() };
}
