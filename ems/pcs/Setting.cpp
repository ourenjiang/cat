#include "Setting.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/base/OperationRecord.h"
#include "ems/base/UserManager.h"
#include "utils/datetime.h"
#include "utils/jsonWrapper.h"
#include "utils/zeromq.h"
#include "utils/StreamWrapper.h"
#include "utils/ModbusRtu.h"

using namespace ems;
using namespace ems::pcs;

void Setting::requestCallback(const Request &req, Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    const auto body = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(body);/* 鉴权 */
    const string branchIndex = base::UserManager::getParam(body, "branchIndex");
    const string controlType = base::UserManager::getParam(body, "type");

    // 请求
    stationDealer->send(zmq::message_t(string("PCS0")), zmq::send_flags::sndmore);// devId
    stationDealer->send(zmq::message_t(string("Interface")), zmq::send_flags::sndmore);// return id

    uint16_t regAddress{}, regData{};
    if(controlType == "charge" || controlType == "discharge"){
        regAddress = 0x04C4;

        const string powerStr = base::UserManager::getParam(body, "power");
        const double powerVal = std::stod(powerStr);
        regData = static_cast<uint16_t>(powerVal * 0.1);//单位:0.1kW
        if(controlType == "charge") regData *= (-1);
    }
    else if(controlType == "standby"){
        regAddress = 0x04C4;
        regData = 0;
    }
    else if(controlType == "powerOff"){
        regAddress = 0x04CC;
        regData = 0xAAAA;
    }
    else if(controlType == "powerOn"){
        regAddress = 0x04CC;
        regData = 0x5555;
    }
    else
        throw std::runtime_error("request params err");
    stationDealer->send(createMsg(regAddress, regData), zmq::send_flags::none);

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