#include "MonthPlan.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "sqlite_modern_cpp.h"
#include "json/json.h"
#include <memory>
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/station/OperationRecord.h"
#include "ems/station/UserManager.h"
#include "ems/station/AuthException.h"

using namespace ems;
using namespace ems::electricity_price;

MonthPlan::MonthPlan()
{
    createTable();
    insertIntoDefaultRecord();
    registerHttpInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void MonthPlan::registerHttpInterfaces()
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    auto& serv = utils::getHttpServerSingleton();
    serv.Post("/xftg/electricityPrice/monthPlan", httplib::Server::Handler(bind(&MonthPlan::requestCallbackPost, this, _1, _2)));
    serv.Get("/xftg/electricityPrice/monthPlan", httplib::Server::Handler(bind(&MonthPlan::requestCallbackGet, this, _1, _2)));
    serv.Delete("/xftg/electricityPrice/monthPlan", httplib::Server::Handler(bind(&MonthPlan::requestCallbackDelete, this, _1, _2)));
    serv.Put("/xftg/electricityPrice/monthPlan", httplib::Server::Handler(bind(&MonthPlan::requestCallbackPut, this, _1, _2)));
}

vector<byte> MonthPlan::respondCallbackPost(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 这里需要从数据库重新加载这部分记录.

    // 返回结果
    const string result{ "success" };
    return { reinterpret_cast<const byte*>(result.data()),
            reinterpret_cast<const byte*>(result.data()) + result.size() };
}

vector<byte> MonthPlan::respondCallbackDelete(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 这里需要从数据库重新加载这部分记录.

    const string successMsg{ "success" };
    return { reinterpret_cast<const byte*>(successMsg.data()),
                reinterpret_cast<const byte*>(successMsg.data()) + successMsg.size() };    
}

vector<byte> MonthPlan::respondCallbackPut(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 这里需要从数据库重新加载这部分记录.

    // 返回结果
    const string result{ "success" };
    return { reinterpret_cast<const byte*>(result.data()),
            reinterpret_cast<const byte*>(result.data()) + result.size() };
}

void MonthPlan::requestCallbackPost(const httplib::Request &req, httplib::Response &res)
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
    if(!reqbody.isMember("name") || !reqbody.isMember("monthNo")
        || !reqbody.isMember("dayPlanName") || !reqbody.isMember("typeListName")){
        throw std::runtime_error("request params err");
    }
    const string monthPlanName = reqbody["name"].asString();
    const string monthNo = reqbody["monthNo"].asString();
    const string dayPlanName = reqbody["dayPlanName"].asString();
    const string typeListName = reqbody["typeListName"].asString();
    
    // 操作数据库
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    // 检查记录是否已存在
    int monthPlanRecordCount{ 0 };
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_MONTHPLAN WHERE NAME = ?;"
        << monthPlanName >> monthPlanRecordCount;
    if(monthPlanRecordCount > 0)
        throw AuthException("月计划已存在", auth["username"].asString());
    
    // 检查DayPlan记录是否已存在
    int dayPlanRecordCount{ 0 };
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_DAYPLAN WHERE NAME = ?;"
        << dayPlanName >> dayPlanRecordCount;
    if(dayPlanRecordCount == 0)
        throw AuthException("日计划不存在", auth["username"].asString());
    
    // 检查TypeList记录是否已存在
    int typeListRecordCount{ 0 };
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_TYPELIST WHERE NAME = ?;"
        << typeListName >> typeListRecordCount;
    if(typeListRecordCount == 0)
        throw AuthException("电价类型不存在", auth["username"].asString());
    
    ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                        "NAME, "
                        "MONTH_NO, "
                        "DAYPLAN_NAME, TYPELIST_NAME) "
                        "VALUES (?, ?, ?, ?);"
                    << monthPlanName
                    << monthNo << dayPlanName << typeListName;

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("ElectricityPriceMonthPlanPost");
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

    // 保存'失败'操作记录
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

void MonthPlan::requestCallbackGet(const httplib::Request &req, httplib::Response &res)
{
    // 不提供名称时，将请求转发至获取全部记录接口
    if(!req.has_param("name")){
        requestCallbackGetAll(req, res);
        return;
    }

try
{
    const string name = req.get_param_value("name");

    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    // 准备查询结果
    optional<RecordWithName> record;

    // 查询
    ElectricityPriceDb << "SELECT "
                            "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME "
                            "FROM ELECTRICITY_PRICE_MONTHPLAN "
                            "WHERE NAME = ?;"
                        << name
    >> [&](string name, string monthNo, string dayPlanName, string typeListName){
            record.emplace(name, monthNo, dayPlanName, typeListName);
        };
    if(!record.has_value())
        throw std::runtime_error("record not exists");
    auto& [durationName, durationType, durationBegin, durationEnd] = record.value();

    Json::Value root;
    root["name"] = durationName;
    root["monthNo"] = durationType;
    root["dayPlanName"] = durationBegin;
    root["typeListName"] = durationEnd;

    Json::Value respondContent;
    respondContent["data"] = root;
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

void MonthPlan::requestCallbackGetAll(const httplib::Request &req, httplib::Response &res)
{
try
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    // 查询
    vector<RecordWithName> recordList;
    ElectricityPriceDb << "SELECT * FROM ELECTRICITY_PRICE_MONTHPLAN;"
    >> [&](string name, string monthNo, string dayPlanName, string typeListName){
            recordList.emplace_back(name, monthNo, dayPlanName, typeListName);
        };
    if(recordList.empty())
        throw std::runtime_error("record not exists");

    Json::Value jsonArray;
    for(const auto& item: recordList){
        auto& [durationName, durationType, durationBegin, durationEnd] = item;
        Json::Value subElement;
        subElement["name"] = durationName;
        subElement["monthNo"] = durationType;
        subElement["dayPlanName"] = durationBegin;
        subElement["typeListName"] = durationEnd;
        jsonArray.append(subElement);
    }

    Json::Value respondContent;
    respondContent["data"] = jsonArray;
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

void MonthPlan::requestCallbackDelete(const httplib::Request &req, httplib::Response &res)
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
    if(!reqbody.isMember("name") || !reqbody.isMember("durationList"))
        throw std::runtime_error("request params err");
    const auto& durationList = reqbody["durationList"];
    if(durationList.empty() || !durationList.isArray())
        throw std::runtime_error("request params err");
    const string monthPlanName = reqbody["name"].asString();

    // 数据库操作
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    // 检查记录是否存在
    int recordCount{0};
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_MONTHPLAN WHERE NAME = ?;"
        << monthPlanName >> recordCount;
    if(recordCount == 0)
        throw std::runtime_error("record not exists");

    // 执行删除
    ElectricityPriceDb << "DELETE FROM ELECTRICITY_PRICE_MONTHPLAN WHERE NAME = ?;"
                        << monthPlanName;

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("ElectricityPriceMonthPlanDelete");
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
    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

void MonthPlan::requestCallbackPut(const httplib::Request &req, httplib::Response &res)
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
    if(!reqbody.isMember("name") || !reqbody.isMember("monthNo")
        || !reqbody.isMember("dayPlanName") || !reqbody.isMember("typeListName")){
        throw std::runtime_error("request params err");
    }
    const string monthPlanName = reqbody["name"].asString();
    const string monthNo = reqbody["monthNo"].asString();
    const string dayPlanName = reqbody["dayPlanName"].asString();
    const string typeListName = reqbody["typeListName"].asString();

    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    // 查询记录是否存在
    int recordCount{0};
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_MONTHPLAN WHERE NAME = ?;"
        << monthPlanName >> recordCount;
    if(recordCount == 0)
        throw AuthException("月计划不存在", usernameAuth);
    
    // 检查DayPlan记录是否已存在
    int dayPlanRecordCount{ 0 };
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_DAYPLAN WHERE NAME = ?;"
        << dayPlanName >> dayPlanRecordCount;
    if(dayPlanRecordCount == 0)
        throw AuthException("日计划不存在", usernameAuth);
    
    // 检查TypeList记录是否已存在
    int typeListRecordCount{ 0 };
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_TYPELIST WHERE NAME = ?;"
        << typeListName >> typeListRecordCount;
    if(typeListRecordCount == 0)
        throw AuthException("电价类型不存在", usernameAuth);
    
    ElectricityPriceDb << "UPDATE ELECTRICITY_PRICE_MONTHPLAN SET "
                            "MONTH_NO = ?, "
                            "DAYPLAN_NAME = ?, "
                            "TYPELIST_NAME = ? "
                            "WHERE NAME = ?;"
                        << monthNo
                        << dayPlanName << typeListName
                        << monthPlanName;

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("ElectricityPriceMonthPlanPut");
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
    const string status { "success" };
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

std::string MonthPlan::createTable()
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/ElectricityPrice.sqlite" };
        sqlite::database ElectricityPriceDb(filename);

        /**
         * 后期优化注解：
         *      1, 拼接列定义时，可以先使用vector将其缓存，然后再与SQL语句进行组合
         *      2, jian_input等列实际业务数据类型应为浮点类型，当前原型开发阶段暂时统一为字符串类型
         *      3, ...
        */
        ElectricityPriceDb << "CREATE TABLE IF NOT EXISTS ELECTRICITY_PRICE_MONTHPLAN("
                              "NAME TEXT PRIMARY KEY,"
                              "MONTH_NO TEXT,"
                              "DAYPLAN_NAME TEXT,"
                              "TYPELIST_NAME TEXT)";
        return filename;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
}

bool MonthPlan::insertIntoDefaultRecord()
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/ElectricityPrice.sqlite" };
        sqlite::database ElectricityPriceDb(filename);

        ElectricityPriceDb << "DELETE FROM ELECTRICITY_PRICE_MONTHPLAN;";// clear old records

        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan1" << "1" << "dayPlan1" << "typeList1";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan2" << "2" << "dayPlan2" << "typeList1";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan3" << "3" << "dayPlan2" << "typeList2";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan4" << "4" << "dayPlan1" << "typeList1";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan5" << "5" << "dayPlan1" << "typeList2";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan6" << "6" << "dayPlan2" << "typeList2";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan7" << "7" << "dayPlan1" << "typeList1";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan8" << "8" << "dayPlan2" << "typeList1";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan9" << "9" << "dayPlan2" << "typeList2";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan10" << "10" << "dayPlan1" << "typeList1";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan11" << "11" << "dayPlan1" << "typeList2";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                                "NAME, MONTH_NO, DAYPLAN_NAME, TYPELIST_NAME) "
                                "VALUES (?, ?, ?, ?);"
                                << "monthPlan12" << "12" << "dayPlan2" << "typeList2";
        return true;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
}

bool MonthPlan::deleteRecord(const string& name)
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/ElectricityPrice.sqlite" };
        sqlite::database ElectricityPriceDb(filename);

        // 检查记录是否存在
        int recordCount{0};
        ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_MONTHPLAN WHERE NAME = ?;"
            << name >> recordCount;
        if(recordCount == 0)
            throw std::runtime_error("记录不存在");

        // 执行删除
        ElectricityPriceDb << "DELETE FROM ELECTRICITY_PRICE_MONTHPLAN "
                              "WHERE NAME = ?;"
                            << name;
        return true;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
}

bool MonthPlan::modifyRecord(const string& name, const string& monthNo,
                    const string& dayPlanName, const string& typeListName)
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/ElectricityPrice.sqlite" };
        sqlite::database ElectricityPriceDb(filename);

        /** 先查询旧记录 */
        using TempRecord = tuple<string, string, string>;
        std::optional<TempRecord> optValue;

        // 查询
        int recordCount{0};
        ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_MONTHPLAN WHERE NAME = ?;"
            << name >> recordCount;
        if(recordCount == 0)
            throw std::runtime_error("记录不存在");
        
        ElectricityPriceDb << "UPDATE ELECTRICITY_PRICE_MONTHPLAN SET "
                              "MONTH_NO = ?, "
                              "DAYPLAN_NAME = ?, "
                              "TYPELIST_NAME = ? "
                              "WHERE NAME = ?;"
                           << monthNo
                           << dayPlanName << typeListName
                           << name;
        return true;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
}

std::optional<MonthPlan::Record> MonthPlan::getRecord(const string& name)
{
    Json::Value finalResult;
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/ElectricityPrice.sqlite" };
        sqlite::database ElectricityPriceDb(filename);

        std::optional<std::tuple<string, string, string>> optValue;

        // 查询
        ElectricityPriceDb << "SELECT "
                              "MONTH_NO, "
                              "DAYPLAN_NAME, TYPELIST_NAME "
                              "FROM ELECTRICITY_PRICE_MONTHPLAN "
                              "WHERE NAME = ?;"
                            << name
        >> [&](string monthNo_,
                string durationName_, string priceName_){
                    
                    optValue.emplace(monthNo_, durationName_, priceName_);
                };
        return optValue;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
}

optional<vector<MonthPlan::RecordWithName>> MonthPlan::getAllRecord()
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/ElectricityPrice.sqlite" };
        sqlite::database ElectricityPriceDb(filename);

        // 准备查询结果
        vector<RecordWithName> records;

        // 查询
        ElectricityPriceDb << "SELECT "
                              "NAME, "
                              "MONTH_NO, "
                              "DAYPLAN_NAME, TYPELIST_NAME "
                              "FROM ELECTRICITY_PRICE_MONTHPLAN;"
        >> [&](string name, 
                string monthNo,
                string dayPlanName, string typeListName){
                    
                    records.emplace_back(name, monthNo, dayPlanName, typeListName);
                };
        
        if(!records.empty())
            return optional<vector<RecordWithName>>(records);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    return {};
}
