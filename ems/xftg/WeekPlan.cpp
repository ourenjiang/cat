#include "WeekPlan.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include <regex>
#include "ems/base/UserManager.h"
#include "ems/base/OperationRecord.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/AuthException.h"
#include "utils/datetime.h"
#include "utils/jsonWrapper.h"

using namespace ems;
using namespace ems::xftg;

void WeekPlan::createTable()
{
try
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    XftgDb << "CREATE TABLE IF NOT EXISTS XFTG_WEEK_PLAN("
                            "NAME TEXT PRIMARY KEY,"
                            "DAYPLAN_DURATION_NAME TEXT,"
                            "DAYPLAN_PROTECT_NAME TEXT,"
                            "DAY_WHITE_LIST TEXT,"
                            "VALID_DATE_BEGIN TEXT,"
                            "VALID_DATE_END TEXT, "
                            "PRIORITY TEXT, "
                            "BIND_SYSTEM TEXT);";
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void WeekPlan::insertIntoDefaultRecord()
{
try
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    // XftgDb << "DELETE FROM XFTG_WEEK_PLAN;";// clear old records
    int recordCount{0};
    XftgDb << "SELECT COUNT(*) FROM XFTG_WEEK_PLAN;" >> recordCount;
    if(recordCount > 0) return;

    XftgDb << "INSERT INTO XFTG_WEEK_PLAN ("
                "NAME, "
                "DAYPLAN_DURATION_NAME, "
                "DAYPLAN_PROTECT_NAME, "
                "DAY_WHITE_LIST, "
                "VALID_DATE_BEGIN, VALID_DATE_END, "
                "PRIORITY, BIND_SYSTEM) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
                << "weekPlan1"
                << "dayPlanDuration1" << "dayPlanProtect1"
                << createDayOfWeekListString({ 1, 2, 3, 4, 5, 6, 7 })
                << "1990-01-01" << "2050-01-01"
                << "level1" << "branch1";
    XftgDb << "INSERT INTO XFTG_WEEK_PLAN ("
                "NAME, "
                "DAYPLAN_DURATION_NAME, "
                "DAYPLAN_PROTECT_NAME, "
                "DAY_WHITE_LIST, "
                "VALID_DATE_BEGIN, VALID_DATE_END, "
                "PRIORITY, BIND_SYSTEM) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
                << "weekPlan2"
                << "dayPlanDuration2" << "dayPlanProtect2"
                << createDayOfWeekListString({ 1, 2, 3, 4, 5, 6, 7 })
                << "2024-01-01" << "2025-01-01"
                << "level2" << "branch1";
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void WeekPlan::requestCallbackPost(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */

    /** 解析业务参数 */
    if(!reqbody.isMember("name") 
        || !reqbody.isMember("dayPlanDurationName") || !reqbody.isMember("dayPlanProtectName") 
        || !reqbody.isMember("dayWhiteList") || !reqbody.isMember("validDateBegin") 
        || !reqbody.isMember("validDateEnd") || !reqbody.isMember("priority")
        || !reqbody.isMember("bindSystem"))
        throw std::runtime_error("request params err");
    
    const string weekPlanName = reqbody["name"].asString();
    const string dayPlanDurationName = reqbody["dayPlanDurationName"].asString();
    const string dayPlanProtectName = reqbody["dayPlanProtectName"].asString();
    const string dayWhiteList = reqbody["dayWhiteList"].asString();
    const string validDateBegin = reqbody["validDateBegin"].asString();
    const string validDateEnd = reqbody["validDateEnd"].asString();
    const string priority = reqbody["priority"].asString();
    const string bindSystem = reqbody["bindSystem"].asString();

    // 操作数据库
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    {
        // 检查记录是否已存在
        int recordCount{ 0 };
        XftgDb << "SELECT COUNT(*) FROM XFTG_WEEK_PLAN WHERE NAME = ?;"
            << weekPlanName >> recordCount;
        if(recordCount > 0)
            throw AuthException("记录已存在", usernameAuth);
    }
    {
        // 检查DayPlan记录是否已存在
        int recordCount{ 0 };
        XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_DURATION WHERE NAME = ?;"
            << dayPlanDurationName >> recordCount;
        if(recordCount == 0)
            throw AuthException("日计划不存在", usernameAuth);
    }
    {
        // 检查DayPlan记录是否已存在
        int recordCount{ 0 };
        XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_PROTECT WHERE NAME = ?;"
            << dayPlanProtectName >> recordCount;
        if(recordCount == 0)
            throw AuthException("保护计划不存在", usernameAuth);
    }

    XftgDb << "INSERT INTO XFTG_WEEK_PLAN ("
                "NAME, "
                "DAYPLAN_DURATION_NAME, "
                "DAYPLAN_PROTECT_NAME, "
                "DAY_WHITE_LIST, "
                "VALID_DATE_BEGIN, VALID_DATE_END, "
                "PRIORITY, BIND_SYSTEM) "
                "VALUES (?, ?, ?, ?, ?, ?, ?, ?);"
            << weekPlanName
            << dayPlanDurationName << dayPlanProtectName
            << dayWhiteList << validDateBegin << validDateEnd
            << priority << bindSystem;

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
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void WeekPlan::requestCallbackGet(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const string weekPlanName = req.get_param_value("name");

    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    std::optional<Record> record;

    // 查询
    XftgDb << "SELECT "
                "DAYPLAN_DURATION_NAME, DAYPLAN_PROTECT_NAME, "
                "DAY_WHITE_LIST, VALID_DATE_BEGIN, VALID_DATE_END, PRIORITY, BIND_SYSTEM "
                "FROM XFTG_WEEK_PLAN WHERE NAME = ?;"
            << weekPlanName
    >> [&](string dayPlanDurationName, string dayPlanProtectName, string dayWhiteList,
            string validDateBegin, string validDateEnd, string priority, string bindSystem){
                record.emplace(dayPlanDurationName, dayPlanProtectName, dayWhiteList,
                                validDateBegin, validDateEnd, priority, bindSystem);
            };
    if(!record.has_value())
        throw std::runtime_error("record not exists");
    auto& [durationName, protectName, dayWhiteList,
            validDateBegin, validDateEnd, priority, bindSystem] = record.value();

    Json::Value root;
    root["dayPlanDurationName"] = durationName;
    root["dayPlanProtectName"] = protectName;
    root["dayWhiteList"] = dayWhiteList;
    root["validDateBegin"] = validDateBegin;
    root["validDateEnd"] = validDateEnd;
    root["priority"] = priority;
    root["bindSystem"] = bindSystem;

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

void WeekPlan::requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
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
    XftgDb << "SELECT NAME FROM XFTG_WEEK_PLAN;"
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

void WeekPlan::requestCallbackDelete(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */

    /** 解析业务参数 */
    if(!reqbody.isMember("name"))
        throw std::runtime_error("request params err");
    const string weekPlanName = reqbody["name"].asString();

    // 数据库操作
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    // 检查记录是否存在
    int recordCount{0};
    XftgDb << "SELECT COUNT(*) FROM XFTG_WEEK_PLAN WHERE NAME = ?;"
        << weekPlanName >> recordCount;
    if(recordCount == 0)
        throw std::runtime_error("记录不存在");

    // 执行删除
    XftgDb << "DELETE FROM XFTG_WEEK_PLAN WHERE NAME = ?;" << weekPlanName;

    // stationDealer->send(zmq::message_t(deleteSubtitle_), zmq::send_flags::sndmore);
    // stationDealer->send(zmq::message_t(), zmq::send_flags::none);

    Json::Value respondContent;
    respondContent["errcode"] = 0;
    respondContent["errmsg"] = "success";
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e)
{
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void WeekPlan::requestCallbackPut(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */

    /** 解析业务参数 */
    if(!reqbody.isMember("name") 
        || !reqbody.isMember("dayPlanDurationName") || !reqbody.isMember("dayPlanProtectName") 
        || !reqbody.isMember("dayWhiteList") || !reqbody.isMember("validDateBegin") 
        || !reqbody.isMember("validDateEnd") || !reqbody.isMember("priority")
        || !reqbody.isMember("bindSystem"))
        throw std::runtime_error("request params err");
    
    const string weekPlanName = reqbody["name"].asString();
    const string dayPlanDurationName = reqbody["dayPlanDurationName"].asString();
    const string dayPlanProtectName = reqbody["dayPlanProtectName"].asString();
    const string dayWhiteList = reqbody["dayWhiteList"].asString();
    const string validDateBegin = reqbody["validDateBegin"].asString();
    const string validDateEnd = reqbody["validDateEnd"].asString();
    const string priority = reqbody["priority"].asString();
    const string bindSystem = reqbody["bindSystem"].asString();

    // 操作数据库
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    {
        // 查询周计划记录是否存在
        int recordCount{0};
        XftgDb << "SELECT COUNT(*) FROM XFTG_WEEK_PLAN WHERE NAME = ?;"
            << weekPlanName >> recordCount;
        if(recordCount == 0)
            throw AuthException("月计划不存在", usernameAuth);
    }
    {
        // 查询日计划记录是否存在
        int recordCount{0};
        XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_DURATION WHERE NAME = ?;"
            << dayPlanDurationName >> recordCount;
        if(recordCount == 0)
            throw AuthException("日计划不存在", usernameAuth);
    }
    {
        // 查询保护参数记录是否存在
        int recordCount{0};
        XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_PROTECT WHERE NAME = ?;"
            << dayPlanProtectName >> recordCount;
        if(recordCount == 0)
            throw AuthException("保护参数不存在", usernameAuth);
    }

    XftgDb << "UPDATE XFTG_WEEK_PLAN SET "
                            "DAYPLAN_DURATION_NAME = ?, "
                            "DAYPLAN_PROTECT_NAME = ?, "
                            "DAY_WHITE_LIST = ?, "
                            "VALID_DATE_BEGIN = ?, VALID_DATE_END = ?, "
                            "PRIORITY = ?, BIND_SYSTEM = ? "
                            "WHERE NAME = ?;"
                        << dayPlanDurationName << dayPlanProtectName
                        << dayWhiteList << validDateBegin << validDateEnd
                        << priority << bindSystem
                        << weekPlanName;

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
    // OperationRecord::insertRecord("失败", "修改削峰填俗-周计划 ", "用户管理", usernameAuth);

    //记录错误操作日志
    auto authException = dynamic_cast<const AuthException*>(&e);
    if(authException){
        base::OperationRecord::insertRecord("失败", authException->what(), "参数设置", authException->username());
    }

    // 失败响应
    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}   
}

string WeekPlan::createDayOfWeekListString(const vector<int>& dayOfWeekList)
{
    string body;
    for(const int item : dayOfWeekList){
        body += to_string(item);
    }
    if(dayOfWeekList.size() > 1){
        auto beginItr = dayOfWeekList.begin();
        const auto endItr = dayOfWeekList.end() - 1;
        for(auto itr = beginItr; itr != endItr; itr++){
            const string target = to_string(*itr);
            auto targetPos = body.find(target);
            body.insert(body.begin() + targetPos + 1, ',');
        }
    }
    return '[' + body + ']';
}

vector<int> WeekPlan::convertDayofWeekListfromString(const string& data)
{
    vector<int> dayOfWeekList;
    // for(size_t i = 0; i < data.size(); ++i){
    //     const size_t pos = data.find(',', i);
    //     if(pos == string::npos){
    //         break;
    //     }

    //     const string subItem = data.substr(pos - 1, 1);
    //     dayOfWeekList.push_back(std::stoi(subItem));
    //     i += (pos + 1);
    // }

    // const string border{ "[]" };
    // if(data.size() > border.size()){
    //     const size_t lastElementPos = data.size() - 2;
    //     const string subItem = data.substr(lastElementPos, 1);
    //     dayOfWeekList.push_back(std::stoi(subItem));
    // }
    // return dayOfWeekList;

    std::regex pattern(R"(\d+)");
    std::sregex_iterator itr(data.begin(), data.end(), pattern);
    std::sregex_iterator end;

    for(; itr != end; ++itr){
        dayOfWeekList.push_back(std::stoi(itr->str()));
    }
    return dayOfWeekList;
}

std::optional<WeekPlan::Record> WeekPlan::getRecord(const string& name)
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Xftg.sqlite" };
        sqlite::database XftgDb(filename);

        std::optional<Record> optValue;

        // 查询
        XftgDb << "SELECT "
                    "DAYPLAN_DURATION_NAME, "
                    "DAYPLAN_PROTECT_NAME, "
                    "DAY_WHITE_LIST, "
                    "VALID_DATE_BEGIN, VALID_DATE_END, "
                    "PRIORITY, BIND_SYSTEM "
                    "FROM XFTG_WEEK_PLAN "
                    "WHERE NAME = ?;"
                << name

        >> [&](string dayPlanDurationName_, string dayPlanProtectName_,
                string dayWhiteList_,
                string validDateBegin_, string validDateEnd_,
                string priority_, string bindSystem_){
                    
                    optValue.emplace(dayPlanDurationName_, dayPlanProtectName_,
                                        dayWhiteList_,
                                        validDateBegin_, validDateEnd_,
                                        priority_, bindSystem_);
                };
        return optValue;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
}

std::optional<vector<WeekPlan::RecordWithName>> WeekPlan::getAllRecords()
{
try
{
    /* code */
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    vector<RecordWithName> RecordWithNameList;

    // 查询
    XftgDb << "SELECT * FROM XFTG_WEEK_PLAN;"
                    >> [&](string name, string dayPlanDurationName, string dayPlanProtectName,
                            string dayWhiteList,
                            string validDateBegin, string validDateEnd,
                            string priority, string bindSystem){
                
                        RecordWithNameList.emplace_back(name, dayPlanDurationName, dayPlanProtectName,
                                            dayWhiteList, validDateBegin, validDateEnd,
                                            priority, bindSystem);
                        };
    if(!RecordWithNameList.empty()){
        std::optional<vector<RecordWithName>> optValue;
        optValue.emplace(RecordWithNameList);
        return optValue;
    }
    return {};
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
    return {};
}
