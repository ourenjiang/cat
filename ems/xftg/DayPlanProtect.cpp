#include "DayPlanProtect.h"
#include "DayPlanProtect.h"
#include "boost/assert.hpp"
#include <filesystem>
#include "sqlite_modern_cpp.h"
#include "ems/station/UserManager.h"
#include "ems/station/AuthException.h"
#include "ems/station/OperationRecord.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"

using namespace ems::xftg;

DayPlanProtect::DayPlanProtect()
{
    createTable();
    insertIntoDefaultRecord();
    registerHttpInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void DayPlanProtect::registerHttpInterfaces()
{
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Post("/strategy/protectParams", httplib::Server::Handler(bind(&DayPlanProtect::requestCallbackPost, this, _1, _2)));
    serv.Get("/strategy/protectParams", httplib::Server::Handler(bind(&DayPlanProtect::requestCallbackGet, this, _1, _2)));
    serv.Get("/strategy/protectParams/namelist", httplib::Server::Handler(bind(&DayPlanProtect::requestCallbackGetNameList, this, _1, _2)));
    serv.Delete("/strategy/protectParams", httplib::Server::Handler(bind(&DayPlanProtect::requestCallbackDelete, this, _1, _2)));
    serv.Put("/strategy/protectParams", httplib::Server::Handler(bind(&DayPlanProtect::requestCallbackPut, this, _1, _2)));
}

void DayPlanProtect::requestCallbackPost(const httplib::Request &req, httplib::Response &res)
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
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    // 检查记录是否已存在
    int recordCount{ 0 };
    XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_PROTECT WHERE NAME = ?;"
        << dayPlanName >> recordCount;
    if(recordCount > 0)
        throw AuthException("记录已存在", auth["username"].asString());

    XftgDb << "INSERT INTO XFTG_DAYPLAN_PROTECT ("
                "NAME, "
                "SOC_MAX, SOC_MIN, "
                "TRANSFORMER_POWER_MAX, "
                "POWER_STEP_SIZE, "
                "CHARGE_POWER_MAX, DISCHARGE_POWER_MAX) "
                "VALUES (?, ?, ?, ?, ?, ?, ?);"
            << dayPlanName
            << socMax << socMin
            << transformerPowerMax << powerStepSize
            << chargePowerMax << dischargePowerMax;

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("XftgDayPlanProtectPost");
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
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void DayPlanProtect::requestCallbackGet(const httplib::Request &req, httplib::Response &res)
{
try
{
    const string dayPlanName = req.get_param_value("name");

    string name = req.get_param_value("name");
    const auto getResult = getRecord(name);
    if(!getResult.has_value())
        throw std::runtime_error("record not exists");

    Record record = getResult.value();
    Json::Value root;
    root["name"] = dayPlanName;
    root["socMax"] = std::get<0>(record);
    root["socMin"] = std::get<1>(record);
    root["transformerPowerMax"] = std::get<2>(record);
    root["powerStepSize"] = std::get<3>(record);
    root["chargePowerMax"] = std::get<4>(record);
    root["dischargePowerMax"] = std::get<5>(record);

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

void DayPlanProtect::requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res)
{
try
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    // 查询
    Json::Value namelist(Json::arrayValue);
    XftgDb << "SELECT "
                "DISTINCT NAME "// 注意去重
                "FROM XFTG_DAYPLAN_PROTECT;"
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

void DayPlanProtect::requestCallbackDelete(const httplib::Request &req, httplib::Response &res)
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
    if(!reqbody.isMember("name"))
        throw std::runtime_error("request params err");
    const string dayPlanName = reqbody["name"].asString();

    // 数据库操作
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

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

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("XftgDayPlanProtectDelete");
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
    pair<bool, vector<byte>> standardRespondMsg;
    const bool standardUnpackResult = msgpackWrapper::unpack(recvmsg.data(), recvmsg.size(), standardRespondMsg);
    BOOST_ASSERT(standardUnpackResult);
    const auto& [returnStatus, returnContent] = standardRespondMsg;

    // 再解析自定义的响应内容
    if(!returnStatus){
        const string errmsg(reinterpret_cast<const char*>(returnContent.data()),
                            reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());
        throw std::runtime_error(errmsg);
    }

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

void DayPlanProtect::requestCallbackPut(const httplib::Request &req, httplib::Response &res)
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
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    // 检查记录是否已存在
    int recordCount{ 0 };
    XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_PROTECT WHERE NAME = ?;"
        << dayPlanName >> recordCount;
    if(recordCount == 0)
        throw AuthException("记录不存在", auth["username"].asString());

    XftgDb << "UPDATE XFTG_DAYPLAN_PROTECT SET "
                        "SOC_MAX = ?, SOC_MIN = ?, "
                        "TRANSFORMER_POWER_MAX = ?, POWER_STEP_SIZE = ?, "
                        "CHARGE_POWER_MAX = ?, DISCHARGE_POWER_MAX = ? "
                        "WHERE NAME = ?;"
                    << socMax << socMin
                    << transformerPowerMax << powerStepSize
                    << chargePowerMax << dischargePowerMax
                    << dayPlanName;

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("XftgDayPlanProtectPut");
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
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

std::string DayPlanProtect::createTable()
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Xftg.sqlite" };
        sqlite::database XftgDb(filename);

        XftgDb << "CREATE TABLE IF NOT EXISTS XFTG_DAYPLAN_PROTECT("
                    "NAME TEXT PRIMARY KEY,"
                    "SOC_MAX TEXT,"
                    "SOC_MIN TEXT,"
                    "TRANSFORMER_POWER_MAX TEXT,"
                    "POWER_STEP_SIZE TEXT,"
                    "CHARGE_POWER_MAX TEXT, "
                    "DISCHARGE_POWER_MAX TEXT);";
        return filename;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
}

bool DayPlanProtect::insertIntoDefaultRecord()
{
    bool finalResult;
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Xftg.sqlite" };
        sqlite::database XftgDb(filename);

        XftgDb << "DELETE FROM XFTG_DAYPLAN_PROTECT;";// clear old records

        XftgDb << "INSERT INTO XFTG_DAYPLAN_PROTECT ("
                    "NAME, "
                    "SOC_MAX, SOC_MIN, "
                    "TRANSFORMER_POWER_MAX, "
                    "POWER_STEP_SIZE, "
                    "CHARGE_POWER_MAX, DISCHARGE_POWER_MAX) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?);"
                << "dayPlanProtect1"
                << "100%" << "10%"
                << "100kW" << "5kW"
                << "25kW" << "15kW";

        XftgDb << "INSERT INTO XFTG_DAYPLAN_PROTECT ("
                    "NAME, "
                    "SOC_MAX, SOC_MIN, "
                    "TRANSFORMER_POWER_MAX, "
                    "POWER_STEP_SIZE, "
                    "CHARGE_POWER_MAX, DISCHARGE_POWER_MAX) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?);"
                << "dayPlanProtect2"
                << "90%" << "20%"
                << "50kW" << "1kW"
                << "20kW" << "30kW";
        return true;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
}

std::optional<DayPlanProtect::Record> DayPlanProtect::getRecord(const string& name)
{
try
{
    /* code */
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    // 准备结果集
    std::optional<Record> optValue;

    // 查询
    XftgDb << "SELECT "
                "SOC_MAX, SOC_MIN, "
                "TRANSFORMER_POWER_MAX, "
                "POWER_STEP_SIZE, "
                "CHARGE_POWER_MAX, DISCHARGE_POWER_MAX "
                "FROM XFTG_DAYPLAN_PROTECT "
                "WHERE NAME = ?;"
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

vector<byte> DayPlanProtect::respondCallbackDelete(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 这里需要从数据库重新加载这部分记录.

    return miscellaneous::convertStringToBytes("success");
}

vector<byte> DayPlanProtect::respondCallbackPost(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 这里需要从数据库重新加载这部分记录.

    return miscellaneous::convertStringToBytes("success");
}

vector<byte> DayPlanProtect::respondCallbackPut(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 这里需要从数据库重新加载这部分记录.

    return miscellaneous::convertStringToBytes("success");
}

std::optional<map<string, DayPlanProtect::Record>> DayPlanProtect::getAllRecord()
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Xftg.sqlite" };
        sqlite::database XftgDb(filename);

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
