#include "DayPlanProtect.h"
#include "DayPlanProtect.h"
#include "boost/assert.hpp"
#include <filesystem>
#include "sqlite_modern_cpp.h"
#include "ems/base/UserManager.h"
#include "ems/interface/OperationRecord.h"
#include "ems/base/OperationRecord.h"
#include "utils/AuthException.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/datetime.h"
#include "utils/jsonWrapper.h"
#include "ems/base/Database.h"

using namespace ems;
using namespace ems::xftg;

void DayPlanProtect::createTable()
{
try
{
    auto XftgDb = base::Database::open("/Xftg.sqlite");
    XftgDb << "CREATE TABLE IF NOT EXISTS XFTG_DAYPLAN_PROTECT("
                "NAME TEXT PRIMARY KEY,"
                "SOC_MAX TEXT,"
                "SOC_MIN TEXT,"
                "TRANSFORMER_POWER_MAX TEXT,"
                "POWER_STEP_SIZE TEXT,"
                "CHARGE_POWER_MAX TEXT, "
                "DISCHARGE_POWER_MAX TEXT);";
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void DayPlanProtect::insertIntoDefaultRecord()
{
try
{
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    int recordCount{0};
    XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_PROTECT;" >> recordCount;
    if(recordCount > 0) return;

    XftgDb << "INSERT INTO XFTG_DAYPLAN_PROTECT ("
                "NAME, SOC_MAX, SOC_MIN, TRANSFORMER_POWER_MAX, "
                "POWER_STEP_SIZE, CHARGE_POWER_MAX, DISCHARGE_POWER_MAX) "
                "VALUES (?, ?, ?, ?, ?, ?, ?);"
            << "dayPlanProtect1" << "100%" << "10%" << "100kW" << "5kW" << "25kW" << "15kW";
    XftgDb << "INSERT INTO XFTG_DAYPLAN_PROTECT ("
                "NAME, SOC_MAX, SOC_MIN, TRANSFORMER_POWER_MAX, "
                "POWER_STEP_SIZE, CHARGE_POWER_MAX, DISCHARGE_POWER_MAX) "
                "VALUES (?, ?, ?, ?, ?, ?, ?);"
            << "dayPlanProtect2" << "90%" << "20%" << "50kW" << "1kW" << "20kW" << "30kW";
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void DayPlanProtect::requestCallbackPost(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const auto usernameAuth = base::UserManager::doAuth(reqbody);

    /** 解析业务参数 */
    if(!reqbody.isMember("name") 
        || !reqbody.isMember("socMax") || !reqbody.isMember("socMin") 
        || !reqbody.isMember("transformerPowerMax") || !reqbody.isMember("powerStepSize") 
        || !reqbody.isMember("chargePowerMax") || !reqbody.isMember("dischargePowerMax"))
        throw std::runtime_error("request params err");
    
    const string dayPlanName = reqbody["name"].asString();
    const string socMax = reqbody["socMax"].asString();
    const string socMin = reqbody["socMin"].asString();
    const string transformerPowerMax = reqbody["transformerPowerMax"].asString();
    const string powerStepSize = reqbody["powerStepSize"].asString();
    const string chargePowerMax = reqbody["chargePowerMax"].asString();
    const string dischargePowerMax = reqbody["dischargePowerMax"].asString();

    // 操作数据库
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    // 检查记录是否已存在
    int recordCount{ 0 };
    XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_PROTECT WHERE NAME = ?;"
        << dayPlanName >> recordCount;
    if(recordCount > 0)
        throw AuthException("记录已存在", usernameAuth);

    XftgDb << "INSERT INTO XFTG_DAYPLAN_PROTECT ("
                "NAME, SOC_MAX, SOC_MIN, TRANSFORMER_POWER_MAX, "
                "POWER_STEP_SIZE, CHARGE_POWER_MAX, DISCHARGE_POWER_MAX) "
                "VALUES (?, ?, ?, ?, ?, ?, ?);"
            << dayPlanName
            << socMax << socMin
            << transformerPowerMax << powerStepSize
            << chargePowerMax << dischargePowerMax;

    // 通知站点重新加载数据
    cmdDealer->send(zmq::message_t(string("Strategy0Set")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("InterfaceCmd")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("DayPlanProtect")), zmq::send_flags::none);

    // 响应
    zmq::message_t deviceBody;
    (void)cmdDealer->recv(deviceBody);

    // 解析消息
    pair<bool, string> respondMsg;
    const bool unpackMsgResult = msgpackWrapper::unpack(deviceBody.data(), deviceBody.size(), respondMsg);
    BOOST_ASSERT(unpackMsgResult);
    const auto& [returnStatus, returnContent] = respondMsg;
    if(!returnStatus)
        throw std::runtime_error("modbus respond failed");
    base::OperationRecord::insertRecord("success", "添加削峰填谷保护计划", "参数设置", usernameAuth);

    // 成功响应
    Json::Value repJson;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e){
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void DayPlanProtect::requestCallbackGet(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const string dayPlanName = req.get_param_value("name");

    string name = req.get_param_value("name");
    const auto getResult = getRecord(name);
    if(!getResult.has_value())
        throw std::runtime_error("record not exists");
    const auto& [socMax, socMin, transformerPowerMax,
        powerStepSize, chargePowerMax, dischargePowerMax] = getResult.value();

    Json::Value root;
    root["name"] = dayPlanName;
    root["socMax"] = socMax;
    root["socMin"] = socMin;
    root["transformerPowerMax"] = transformerPowerMax;
    root["powerStepSize"] = powerStepSize;
    root["chargePowerMax"] = chargePowerMax;
    root["dischargePowerMax"] = dischargePowerMax;

    Json::Value respondContent;
    respondContent["data"] = root;
    respondContent["errcode"] = 0;
    respondContent["errmsg"] = "success";
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){
    Json::Value respondmsg;
    respondmsg["data"] = Json::Value(Json::objectValue);
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

void DayPlanProtect::requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    // 查询
    Json::Value namelist(Json::arrayValue);
    XftgDb << "SELECT DISTINCT NAME FROM XFTG_DAYPLAN_PROTECT;"// 注意去重
            >> [&](string name){
                    namelist.append(name);
                };
    if(namelist.empty())
        throw std::runtime_error("record not exists");

    Json::Value respondContent;
    respondContent["data"] = namelist;
    respondContent["errcode"] = 0;
    respondContent["errmsg"] = "success";
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){
    Json::Value respondmsg;
    respondmsg["data"] = Json::Value(Json::arrayValue);
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

void DayPlanProtect::requestCallbackDelete(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const auto usernameAuth = base::UserManager::doAuth(reqbody);

    /** 解析业务参数 */
    if(!reqbody.isMember("name"))
        throw std::runtime_error("request params err");
    const string dayPlanName = reqbody["name"].asString();

    // 数据库操作
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    {
        // 检查记录是否存在
        int recordCount{0};
        XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_PROTECT WHERE NAME = ?;"
            << dayPlanName >> recordCount;
        if(recordCount == 0)
            throw std::runtime_error("记录不存在");
    }

    {
        // 检查是否有周计划记录绑定了该保护参数；
        int recordCount{0};
        XftgDb << "SELECT COUNT(*) FROM XFTG_WEEK_PLAN WHERE DAYPLAN_PROTECT_NAME = ?;"
            << dayPlanName >> recordCount;
        if(recordCount > 0)
            throw std::runtime_error("记录在周计划中被占用");
    }

    // 执行删除
    XftgDb << "DELETE FROM XFTG_DAYPLAN_PROTECT WHERE NAME = ?;" << dayPlanName;

    // 通知站点重新加载数据
    cmdDealer->send(zmq::message_t(string("Strategy0Set")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("InterfaceCmd")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("DayPlanProtect")), zmq::send_flags::none);

    // 响应
    zmq::message_t deviceBody;
    (void)cmdDealer->recv(deviceBody);

    // 解析消息
    pair<bool, string> respondMsg;
    const bool unpackMsgResult = msgpackWrapper::unpack(deviceBody.data(), deviceBody.size(), respondMsg);
    BOOST_ASSERT(unpackMsgResult);
    const auto& [returnStatus, returnContent] = respondMsg;
    if(!returnStatus)
        throw std::runtime_error("modbus respond failed");
    base::OperationRecord::insertRecord("success", "删除削峰填谷保护计划", "参数设置", usernameAuth);

    Json::Value respondContent;
    respondContent["errcode"] = 0;
    respondContent["errmsg"] = "success";
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void DayPlanProtect::requestCallbackPut(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const auto usernameAuth = base::UserManager::doAuth(reqbody);

    /** 解析业务参数 */
    if(!reqbody.isMember("name") 
        || !reqbody.isMember("socMax") || !reqbody.isMember("socMin") 
        || !reqbody.isMember("transformerPowerMax") || !reqbody.isMember("powerStepSize") 
        || !reqbody.isMember("chargePowerMax") || !reqbody.isMember("dischargePowerMax"))
        throw std::runtime_error("request params err");
    
    const string dayPlanName = reqbody["name"].asString();
    const string socMax = reqbody["socMax"].asString();
    const string socMin = reqbody["socMin"].asString();
    const string transformerPowerMax = reqbody["transformerPowerMax"].asString();
    const string powerStepSize = reqbody["powerStepSize"].asString();
    const string chargePowerMax = reqbody["chargePowerMax"].asString();
    const string dischargePowerMax = reqbody["dischargePowerMax"].asString();
    
    // 数据库操作
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    // 检查记录是否已存在
    int recordCount{ 0 };
    XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_PROTECT WHERE NAME = ?;"
        << dayPlanName >> recordCount;
    if(recordCount == 0)
        throw AuthException("记录不存在", usernameAuth);

    XftgDb << "UPDATE XFTG_DAYPLAN_PROTECT SET "
                "SOC_MAX = ?, SOC_MIN = ?, TRANSFORMER_POWER_MAX = ?, "
                "POWER_STEP_SIZE = ?, CHARGE_POWER_MAX = ?, DISCHARGE_POWER_MAX = ? "
                "WHERE NAME = ?;"
            << socMax << socMin
            << transformerPowerMax << powerStepSize
            << chargePowerMax << dischargePowerMax
            << dayPlanName;

    // 通知站点重新加载数据
    cmdDealer->send(zmq::message_t(string("Strategy0Set")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("InterfaceCmd")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("DayPlanProtect")), zmq::send_flags::none);

    // 响应
    zmq::message_t deviceBody;
    (void)cmdDealer->recv(deviceBody);

    // 解析消息
    pair<bool, string> respondMsg;
    const bool unpackMsgResult = msgpackWrapper::unpack(deviceBody.data(), deviceBody.size(), respondMsg);
    BOOST_ASSERT(unpackMsgResult);
    const auto& [returnStatus, returnContent] = respondMsg;
    if(!returnStatus)
        throw std::runtime_error("modbus respond failed");
    base::OperationRecord::insertRecord("success", "修改削峰填谷保护计划", "参数设置", usernameAuth);

    // 成功响应
    Json::Value repJson;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e){
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

std::optional<DayPlanProtect::Record> DayPlanProtect::getRecord(const string& name)
{
try
{
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    // 准备结果集
    std::optional<Record> optValue;

    // 查询
    XftgDb << "SELECT SOC_MAX, SOC_MIN, TRANSFORMER_POWER_MAX, POWER_STEP_SIZE, "
                "CHARGE_POWER_MAX, DISCHARGE_POWER_MAX FROM XFTG_DAYPLAN_PROTECT WHERE NAME = ?;"
            << name
            >> [&](string socMax_, string socMin_,
                    string transformerPowerMax_, string powerStepSize_,
                    string chargePowerMax_, string dischargePowerMax_){
                        optValue.emplace(socMax_, socMin_, 
                                        transformerPowerMax_, powerStepSize_,
                                        chargePowerMax_, dischargePowerMax_);
                    };
    return optValue;
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
return {};
}

std::optional<map<string, DayPlanProtect::Record>> DayPlanProtect::getAllRecord()
{
try
{
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    // 准备结果集
    map<string, Record> records;

    // 查询
    XftgDb << "SELECT * FROM XFTG_DAYPLAN_PROTECT;"
            >> [&](string name, string socMax, string socMin,
                    string transformerPowerMax, string powerStepSize,
                    string chargePowerMax, string dischargePowerMax){
                        records.emplace(name,
                                        make_tuple(socMax, socMin, 
                                            transformerPowerMax, powerStepSize,
                                            chargePowerMax, dischargePowerMax));
                    };
    return records;
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
    return {};
}
