#include "Setting.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/base/UserManager.h"
#include "ems/base/OperationRecord.h"
#include "utils/datetime.h"
#include "utils/ModbusRtu.h"
#include "utils/jsonWrapper.h"

using namespace ems;
using namespace ems::bau;

Setting::Setting()
{
}

void Setting::requestCallbackPowerOff(const httplib::Request &req, httplib::Response &res,
                                        shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    const auto body = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(body);/* 鉴权 */
    const string branchIndex = base::UserManager::getParam(body, "branchIndex");

    // 请求
    stationDealer->send(zmq::message_t(string("BAU0")), zmq::send_flags::sndmore);// devId
    stationDealer->send(zmq::message_t(string("Interface")), zmq::send_flags::sndmore);// return id
    stationDealer->send(createMsg(0xd700, 0), zmq::send_flags::none);
    // 响应
    zmq::message_t deviceBody;
    (void)stationDealer->recv(deviceBody);

    // 解析消息
    pair<bool, vector<uint8_t>> respondMsg;
    const bool unpackMsgResult = msgpackWrapper::unpack(deviceBody.data(), deviceBody.size(), respondMsg);
    BOOST_ASSERT(unpackMsgResult);
    const auto& [returnStatus, returnContent] = respondMsg;
    if(!returnStatus)
        throw std::runtime_error("modbus respond failed");
    base::OperationRecord::insertRecord("success", "BAU关机", "参数设置", usernameAuth);
    respond(res, 0, "success");
}
catch(const std::exception& e){
    base::OperationRecord::insertRecord("failed", e.what(), "参数设置", "username");
    respond(res, -1, e.what());
}
}

void Setting::requestCallbackQuickStartup(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    const auto body = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(body);/* 鉴权 */
    const string branchIndex = base::UserManager::getParam(body, "branchIndex");

    // 请求
    stationDealer->send(zmq::message_t(), zmq::send_flags::sndmore);// Topic
    stationDealer->send(zmq::message_t(string("BAU0")), zmq::send_flags::sndmore);// devId
    stationDealer->send(zmq::message_t(string("Interface")), zmq::send_flags::sndmore);// return id
    stationDealer->send(createMsg(0xd701, 0), zmq::send_flags::none);
    // 响应
    zmq::message_t deviceBody;
    (void)stationDealer->recv(deviceBody);

    // 解析消息
    pair<bool, vector<uint8_t>> respondMsg;
    const bool unpackMsgResult = msgpackWrapper::unpack(deviceBody.data(), deviceBody.size(), respondMsg);
    BOOST_ASSERT(unpackMsgResult);
    const auto& [returnStatus, returnContent] = respondMsg;
    if(!returnStatus)
        throw std::runtime_error("modbus respond failed");
    base::OperationRecord::insertRecord("success", "BAU一键并机", "参数设置", usernameAuth);
    respond(res, 0, "success");
}
catch(const std::exception& e){
    base::OperationRecord::insertRecord("failed", e.what(), "BAU一键并机", "username");
    respond(res, -1, e.what());
}
}

void Setting::requestCallbackSetBcuRelay(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    const auto body = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(body);/* 鉴权 */
    const string branchIndex = base::UserManager::getParam(body, "branchIndex");
    optional<string> bcuIndex;
    if(body.isMember("bcuIndex")){
        bcuIndex = base::UserManager::getParam(body, "bcuIndex");
    }
    const string status = base::UserManager::getParam(body, "status");
    if(status != "on" && status != "off")
        throw std::runtime_error("request params err");
    
    // 请求
    stationDealer->send(zmq::message_t(string("BAU0")), zmq::send_flags::sndmore);// devId
    stationDealer->send(zmq::message_t(string("Interface")), zmq::send_flags::sndmore);// return id
    
    // 准备Modbus请求帧:
    // 0xd701  一键并机   固定0
    // 0xd702  簇分离     簇下标从1开始，0代表所有
    // 0xd703  簇合并     簇下标从1开始，0代表所有
    const uint16_t regAddress = (status == "on") ? 0xd703 : 0xd702;
    uint16_t regData = 0;// 代表所有BCU
    if(bcuIndex.has_value()){
        const string bcuIndexStr = bcuIndex.value();
        uint16_t value = static_cast<uint16_t>(std::stoi(bcuIndexStr));
        regData = value + 1;// 下标 + 1
    }
    stationDealer->send(createMsg(regAddress, regData), zmq::send_flags::none);
    // 响应
    zmq::message_t srcIdentity;
    zmq::message_t deviceBody;
    (void)stationDealer->recv(srcIdentity);
    (void)stationDealer->recv(deviceBody);

    // 解析消息
    pair<bool, vector<uint8_t>> respondMsg;
    const bool unpackMsgResult = msgpackWrapper::unpack(deviceBody.data(), deviceBody.size(), respondMsg);
    BOOST_ASSERT(unpackMsgResult);
    const auto& [returnStatus, returnContent] = respondMsg;
    if(!returnStatus)
        throw std::runtime_error("modbus respond failed");
    base::OperationRecord::insertRecord("success", "设置簇继电器", "参数设置", usernameAuth);
    respond(res, 0, "success");
}
catch(const std::exception& e){
    base::OperationRecord::insertRecord("failed", e.what(), "设置簇继电器", "username");
    respond(res, -1, e.what());
}
}

zmq::message_t Setting::createMsg(const uint16_t regAddress, const uint16_t regData)
{
    auto reqmsg = modbus::modbusRtuWriteFrame(0x01, 0x06, regAddress, regData);
    return { reqmsg.data(), reqmsg.size() };
}

void Setting::respond(Response& res, const int errcode, const string& errmsg)
{
    Json::Value root;
    root["errcode"] = errcode;
    root["errmsg"] = errmsg;
    utils::httpRespond(res, root);
}
