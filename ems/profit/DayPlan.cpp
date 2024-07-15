#include "DayPlan.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "sqlite_modern_cpp.h"
#include "json/json.h"
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

void DayPlan::createTable()
{
try
{
    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");
    ElectricityPriceDb << "CREATE TABLE IF NOT EXISTS ELECTRICITY_PRICE_DAYPLAN("
                            "NAME TEXT, "
                            "DURATION_NAME TEXT, "
                            "DURATION_TYPE TEXT, "
                            "DURATION_BEGIN TEXT, "
                            "DURATION_END TEXT, "
                            "PRIMARY KEY(NAME, DURATION_NAME))";
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void DayPlan::insertIntoDefaultRecord()
{
try
{
    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");

    // 如果存在旧记录，则不添加新的默认记录
    int recordCount{0};
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_DAYPLAN;" >> recordCount;
    if(recordCount > 0) return;

    {
        // 谷时段定义
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                                "NAME, "
                                "DURATION_NAME, DURATION_TYPE, "
                                "DURATION_BEGIN, DURATION_END) "
                                "VALUES (?, ?, ?, ?, ?);"
                            << "dayPlan1"
                            << "duration1"
                            << "gu"
                            << "00:00" << "08:00";
        // 峰时段定义
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                                "NAME, "
                                "DURATION_NAME, DURATION_TYPE, "
                                "DURATION_BEGIN, DURATION_END) "
                                "VALUES (?, ?, ?, ?, ?);"
                            << "dayPlan1"
                            << "duration2"
                            << "feng"
                            << "08:00" << "12:00";
        // 平时段定义
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                                "NAME, "
                                "DURATION_NAME, DURATION_TYPE, "
                                "DURATION_BEGIN, DURATION_END) "
                                "VALUES (?, ?, ?, ?, ?);"
                            << "dayPlan1"
                            << "duration3"
                            << "ping"
                            << "12:00" << "14:00";
        // 峰时段定义
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                                "NAME, "
                                "DURATION_NAME, DURATION_TYPE, "
                                "DURATION_BEGIN, DURATION_END) "
                                "VALUES (?, ?, ?, ?, ?);"
                            << "dayPlan1"
                            << "duration4"
                            << "feng"
                            << "14:00" << "19:00";
        // 尖时段定义
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                                "NAME, "
                                "DURATION_NAME, DURATION_TYPE, "
                                "DURATION_BEGIN, DURATION_END) "
                                "VALUES (?, ?, ?, ?, ?);"
                            << "dayPlan1"
                            << "duration5"
                            << "jian"
                            << "19:00" << "22:00";
        // 谷时段定义
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                                "NAME, "
                                "DURATION_NAME, DURATION_TYPE, "
                                "DURATION_BEGIN, DURATION_END) "
                                "VALUES (?, ?, ?, ?, ?);"
                            << "dayPlan1"
                            << "duration6"
                            << "gu"
                            << "22:00" << "~";
    }
    {
        // 谷时段定义
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                                "NAME, "
                                "DURATION_NAME, DURATION_TYPE, "
                                "DURATION_BEGIN, DURATION_END) "
                                "VALUES (?, ?, ?, ?, ?);"
                            << "dayPlan2"
                            << "duration1"
                            << "gu"
                            << "00:00" << "08:00";
        // 峰时段定义
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                                "NAME, "
                                "DURATION_NAME, DURATION_TYPE, "
                                "DURATION_BEGIN, DURATION_END) "
                                "VALUES (?, ?, ?, ?, ?);"
                            << "dayPlan2"
                            << "duration2"
                            << "feng"
                            << "08:00" << "11:00";
        // 平时段定义
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                                "NAME, "
                                "DURATION_NAME, DURATION_TYPE, "
                                "DURATION_BEGIN, DURATION_END) "
                                "VALUES (?, ?, ?, ?, ?);"
                            << "dayPlan2"
                            << "duration3"
                            << "ping"
                            << "11:00" << "17:00";
        // 尖时段定义
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                                "NAME, "
                                "DURATION_NAME, DURATION_TYPE, "
                                "DURATION_BEGIN, DURATION_END) "
                                "VALUES (?, ?, ?, ?, ?);"
                            << "dayPlan2"
                            << "duration5"
                            << "jian"
                            << "17:00" << "20:00";
        // 谷时段定义
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                                "NAME, "
                                "DURATION_NAME, DURATION_TYPE, "
                                "DURATION_BEGIN, DURATION_END) "
                                "VALUES (?, ?, ?, ?, ?);"
                            << "dayPlan2"
                            << "duration6"
                            << "gu"
                            << "20:00" << "~";
    }
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void DayPlan::requestCallbackPost(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */

    /** 解析业务参数 */
    if(!reqbody.isMember("name") || !reqbody.isMember("durationList"))
        throw std::runtime_error("request params err");
    const auto& durationList = reqbody["durationList"];
    if(durationList.empty() || !durationList.isArray())
        throw std::runtime_error("request params err");
    const string dayPlanName = reqbody["name"].asString();
    
    // 打开数据库
    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");

    // 检查记录是否已存在
    int recordCount{ 0 };
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_DAYPLAN WHERE NAME = ?;"
        << dayPlanName >> recordCount;
    if(recordCount > 0)
        throw AuthException("记录已存在", usernameAuth);

    // 插入数据
    for(const auto& item: durationList){
        if(!item.isMember("durationName") || !item.isMember("durationType")
            || !item.isMember("durationBegin") || !item.isMember("durationEnd")){
            throw std::runtime_error("request params err");
        }
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                            "NAME, DURATION_NAME, DURATION_TYPE, DURATION_BEGIN, DURATION_END) "
                            "VALUES (?, ?, ?, ?, ?);"
                        << dayPlanName
                        << item["durationName"].asString() << item["durationType"].asString()
                        << item["durationBegin"].asString() << item["durationEnd"].asString();
    }

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

void DayPlan::requestCallbackGet(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    if(!req.has_param("name"))
        throw std::runtime_error("request params err");
    const string durationName = req.get_param_value("name");

    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");
    RecordList recordList;

    // 查询
    ElectricityPriceDb << "SELECT "
                            "DURATION_NAME, DURATION_TYPE, DURATION_BEGIN, DURATION_END "
                            "FROM ELECTRICITY_PRICE_DAYPLAN "
                            "WHERE NAME = ?;"
                        << durationName
    >> [&](string durationName, string durationType, string durationBegin, string durationEnd){
            recordList.emplace_back(durationName, durationType, durationBegin, durationEnd);
        };
    if(recordList.empty())
        throw std::runtime_error("record not exists");

    Json::Value jsonArray;
    for(const auto& item: recordList){
        auto& [durationName, durationType, durationBegin, durationEnd] = item;
        Json::Value subElement;
        subElement["durationName"] = durationName;
        subElement["durationType"] = durationType;
        subElement["durationBegin"] = durationBegin;
        subElement["durationEnd"] = durationEnd;
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

void DayPlan::requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");

    // 查询
    NameList namelist;
    ElectricityPriceDb << "SELECT "
                            "DISTINCT NAME "
                            "FROM ELECTRICITY_PRICE_DAYPLAN;"
    >> [&](string name){
            namelist.push_back(name);
        };
    if(namelist.empty())
        throw std::runtime_error("record is empty");

    Json::Value jsonArray(Json::arrayValue);
    for(const auto& item: namelist){
        jsonArray.append(item);
    }

    Json::Value respondContent;
    respondContent["data"] = jsonArray;
    respondContent["errcode"] = 0;
    respondContent["errmsg"] = "success";
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){
    // 失败响应
    Json::Value respondContent;
    respondContent["errcode"] = -1;
    respondContent["errmsg"] = e.what();
    utils::httpRespond(res, respondContent);
}
}

void DayPlan::requestCallbackDelete(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */

    /** 解析业务参数 */
    if(!reqbody.isMember("name"))
        throw std::runtime_error("request params err");
    const string dayPlanName = reqbody["name"].asString();

    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");

    {
        // 检查记录是否存在
        int recordCount{0};
        ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_DAYPLAN WHERE NAME = ?;"
            << dayPlanName >> recordCount;
        if(recordCount == 0)
            throw std::runtime_error("record not exists");
    }

    {
        // 检查是否有月计划记录绑定了该保护参数；
        int recordCount{0};
        ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_MONTHPLAN WHERE DAYPLAN_NAME = ?;"
            << dayPlanName >> recordCount;
        if(recordCount > 0)
            throw std::runtime_error("record is used in month plan");
    }
    
    // 执行删除
    ElectricityPriceDb << "DELETE FROM ELECTRICITY_PRICE_DAYPLAN WHERE NAME = ?;"
                        << dayPlanName;

    // 通知中心
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

void DayPlan::requestCallbackPut(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */

    /** 解析业务参数 */
    if(!reqbody.isMember("name") || !reqbody.isMember("durationList"))
        throw std::runtime_error("request params err");
    const auto& durationList = reqbody["durationList"];
    if(durationList.empty() || !durationList.isArray())
        throw std::runtime_error("request params err");
    const string dayPlanName = reqbody["name"].asString();

    // 打开数据库
    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");

    // 检查记录是否已存在
    int recordCount{ 0 };
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_DAYPLAN WHERE NAME = ?;"
        << dayPlanName >> recordCount;
    if(recordCount == 0)
        throw AuthException("记录不存在", usernameAuth);

    // 插入数据
    for(const auto& item: durationList){
        if(!item.isMember("durationName") || !item.isMember("durationType")
            || !item.isMember("durationBegin") || !item.isMember("durationEnd")){
            throw std::runtime_error("request params err");
        }
        ElectricityPriceDb << "UPDATE ELECTRICITY_PRICE_DAYPLAN SET "
                            "DURATION_TYPE = ?, "
                            "DURATION_BEGIN = ?, DURATION_END = ? "
                            "WHERE NAME = ? AND DURATION_NAME = ?;"
                            << item["durationType"].asString()
                            << item["durationBegin"].asString() << item["durationEnd"].asString()
                            << dayPlanName << item["durationName"].asString();
    }
    
    //通知中心
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

void DayPlan::insertRecord(const string& name,
                            const string& durationName, const string& durationType,
                            const string& durationBegin, const string& durationEnd)
{
    auto ElectricityPriceDb = base::Database::open("/ElectricityPrice.sqlite");
    ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                            "NAME, "
                            "DURATION_NAME, DURATION_TYPE, "
                            "DURATION_BEGIN, DURATION_END) "
                            "VALUES (?, ?, ?, ?, ?);"
                        << name
                        << durationName << durationType
                        << durationBegin << durationEnd;
}
