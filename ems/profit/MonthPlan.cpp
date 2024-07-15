#include "MonthPlan.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "sqlite_modern_cpp.h"
#include "json/json.h"
#include <memory>
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/interface/OperationRecord.h"
#include "ems/base/UserManager.h"
#include "utils/AuthException.h"
#include "utils/datetime.h"
#include "utils/jsonWrapper.h"
#include "ems/base/Database.h"

using namespace ems;
using namespace ems::electricity_price;

void MonthPlan::createTable()
{
try
{
    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");
    ElectricityPriceDb << "CREATE TABLE IF NOT EXISTS ELECTRICITY_PRICE_MONTHPLAN("
                            "NAME TEXT PRIMARY KEY,"
                            "MONTH_NO TEXT,"
                            "DAYPLAN_NAME TEXT,"
                            "TYPELIST_NAME TEXT)";
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void MonthPlan::insertIntoDefaultRecord()
{
try
{
    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");

    // 如果存在旧记录，则不添加新的默认记录
    int recordCount{0};
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_MONTHPLAN;" >> recordCount;
    if(recordCount > 0) return;

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
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void MonthPlan::requestCallbackPost(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */

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
    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");

    // 检查记录是否已存在
    int monthPlanRecordCount{ 0 };
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_MONTHPLAN WHERE NAME = ?;"
        << monthPlanName >> monthPlanRecordCount;
    if(monthPlanRecordCount > 0)
        throw AuthException("月计划已存在", usernameAuth);
    
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
    
    ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_MONTHPLAN ("
                        "NAME, "
                        "MONTH_NO, "
                        "DAYPLAN_NAME, TYPELIST_NAME) "
                        "VALUES (?, ?, ?, ?);"
                    << monthPlanName
                    << monthNo << dayPlanName << typeListName;
    // 通知中心
    // stationDealer->send(zmq::message_t(postSubtitle_), zmq::send_flags::sndmore);
    // stationDealer->send(zmq::message_t(), zmq::send_flags::none);

    // 保存'成功'操作记录
    const string status = "success";
    const string content{ "success" };
    const string type{ "参数设置" };
    const string timestamp = datetime::getCurrentTimestamp();
    const string username = datetime::getCurrentTimestamp();
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
    const string timestamp = datetime::getCurrentTimestamp();
    const string username = datetime::getCurrentTimestamp();
    // OperationRecord::insertRecord(status, content, type, timestamp, username);

    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

void MonthPlan::requestCallbackGet(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
    // 不提供名称时，将请求转发至获取全部记录接口
    if(!req.has_param("name")){
        requestCallbackGetAll(req, res, dataDealer, cmdDealer);
        return;
    }

try
{
    const string name = req.get_param_value("name");

    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");

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

void MonthPlan::requestCallbackGetAll(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");

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

void MonthPlan::requestCallbackDelete(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */

    /** 解析业务参数 */
    if(!reqbody.isMember("name"))
        throw std::runtime_error("request params err");
    const string monthPlanName = reqbody["name"].asString();

    // 数据库操作
    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");

    // 检查记录是否存在
    int recordCount{0};
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_MONTHPLAN WHERE NAME = ?;"
        << monthPlanName >> recordCount;
    if(recordCount == 0)
        throw std::runtime_error("record not exists");

    // 执行删除
    ElectricityPriceDb << "DELETE FROM ELECTRICITY_PRICE_MONTHPLAN WHERE NAME = ?;"
                        << monthPlanName;
    //通知中心
    // stationDealer->send(zmq::message_t(deleteSubtitle_), zmq::send_flags::sndmore);
    // stationDealer->send(zmq::message_t(), zmq::send_flags::none);

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

void MonthPlan::requestCallbackPut(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */

    /** 解析业务参数 */
    if(!reqbody.isMember("name") || !reqbody.isMember("monthNo")
        || !reqbody.isMember("dayPlanName") || !reqbody.isMember("typeListName")){
        throw std::runtime_error("request params err");
    }
    const string monthPlanName = reqbody["name"].asString();
    const string monthNo = reqbody["monthNo"].asString();
    const string dayPlanName = reqbody["dayPlanName"].asString();
    const string typeListName = reqbody["typeListName"].asString();

    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");

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
    // 通知中心
    // stationDealer->send(zmq::message_t(putSubtitle_), zmq::send_flags::sndmore);
    // stationDealer->send(zmq::message_t(), zmq::send_flags::none);

    // 保存'成功'操作记录
    const string status { "success" };
    const string content{ "success" };
    const string type{ "参数设置" };
    const string timestamp = datetime::getCurrentTimestamp();
    const string username = datetime::getCurrentTimestamp();
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
    const string timestamp = datetime::getCurrentTimestamp();
    const string username = datetime::getCurrentTimestamp();
    // OperationRecord::insertRecord(status, content, type, timestamp, username);

    // 失败响应
    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}
