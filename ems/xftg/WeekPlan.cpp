#include "WeekPlan.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include <regex>
#include "ems/station/UserManager.h"
#include "ems/station/AuthException.h"
#include "ems/station/OperationRecord.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"

using namespace ems::xftg;

WeekPlan::WeekPlan()
{
    createTable();
    insertIntoDefaultRecord();
    registerHttpInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void WeekPlan::registerHttpInterfaces()
{
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Post("/strategy/xftg/weekPlan", httplib::Server::Handler(bind(&WeekPlan::requestCallbackPost, this, _1, _2)));
    serv.Get("/strategy/xftg/weekPlan", httplib::Server::Handler(bind(&WeekPlan::requestCallbackGet, this, _1, _2)));
    serv.Get("/strategy/xftg/weekPlan/namelist", httplib::Server::Handler(bind(&WeekPlan::requestCallbackGetNameList, this, _1, _2)));
    serv.Delete("/strategy/xftg/weekPlan", httplib::Server::Handler(bind(&WeekPlan::requestCallbackDelete, this, _1, _2)));
    serv.Put("/strategy/xftg/weekPlan", httplib::Server::Handler(bind(&WeekPlan::requestCallbackPut, this, _1, _2)));
}

vector<byte> WeekPlan::respondCallbackDelete(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 这里需要从数据库重新加载这部分记录.

    return miscellaneous::convertStringToBytes("success");
}

vector<byte> WeekPlan::respondCallbackPost(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 这里需要从数据库重新加载这部分记录.

    return miscellaneous::convertStringToBytes("success");
}

vector<byte> WeekPlan::respondCallbackPut(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 这里需要从数据库重新加载这部分记录.

    return miscellaneous::convertStringToBytes("success");
}

void WeekPlan::requestCallbackPost(const httplib::Request &req, httplib::Response &res)
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

    // 检查记录是否已存在
    int recordCount{ 0 };
    XftgDb << "SELECT COUNT(*) FROM XFTG_WEEK_PLAN WHERE NAME = ?;"
        << weekPlanName >> recordCount;
    if(recordCount > 0)
        throw AuthException("记录已存在", auth["username"].asString());
    
    // 检查DayPlan记录是否已存在
    int dayPlanDurationRecordCount{ 0 };
    XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_DURATION WHERE NAME = ?;"
        << dayPlanDurationName >> dayPlanDurationRecordCount;
    if(dayPlanDurationRecordCount == 0)
        throw AuthException("日计划不存在", auth["username"].asString());
    
    // 检查DayPlan记录是否已存在
    int dayPlanProtectRecordCount{ 0 };
    XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_PROTECT WHERE NAME = ?;"
        << dayPlanDurationName >> dayPlanProtectRecordCount;
    if(dayPlanProtectRecordCount == 0)
        throw AuthException("保护计划不存在", auth["username"].asString());

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

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("XftgWeekPlanPost");
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

void WeekPlan::requestCallbackGet(const httplib::Request &req, httplib::Response &res)
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

void WeekPlan::requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res)
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

void WeekPlan::requestCallbackDelete(const httplib::Request &req, httplib::Response &res)
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

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("XftgWeekPlanDelete");
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
catch(const std::exception& e)
{
    Json::Value msg;
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void WeekPlan::requestCallbackPut(const httplib::Request &req, httplib::Response &res)
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

    // 查询记录是否存在
    int recordCount{0};
    XftgDb << "SELECT COUNT(*) FROM XFTG_WEEK_PLAN WHERE NAME = ?;"
        << weekPlanName >> recordCount;
    if(recordCount == 0)
        throw AuthException("月计划不存在", usernameAuth);

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

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("XftgWeekPlanPut");
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

std::optional<string> WeekPlan::createTable()
{
    try
    {
        /* code */
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
        return filename;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
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

bool WeekPlan::insertIntoDefaultRecord()
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Xftg.sqlite" };
        sqlite::database XftgDb(filename);

        XftgDb << "DELETE FROM XFTG_WEEK_PLAN;";// clear old records

        // Json::Value jsonArray;
        // jsonArray.append(1);
        // jsonArray.append(3);
        // jsonArray.append(5);
        // string jsonArrayString;
        // {
        //     Json::StreamWriterBuilder builder;
        //     builder["emitUTF8"] = true;//输出编码格式
        //     builder["indentation"] = "";//两个空格缩进
        //     jsonArrayString = Json::writeString(builder, jsonArray);
        // }

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
        return true;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
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
