#include "HistoryWarning.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems;

HistoryWarning::HistoryWarning()
{
    createTable();
    registerHttpInterfaces();
}

void HistoryWarning::registerHttpInterfaces()
{
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Post("/historyWarning", httplib::Server::Handler(bind(&HistoryWarning::requestCallbackGet, this, _1, _2)));
    serv.Post("/historyWarningPageInfo", httplib::Server::Handler(bind(&HistoryWarning::requestCallbackGetPageInfo, this, _1, _2)));
}

void HistoryWarning::requestCallbackGet(const httplib::Request &req, httplib::Response &res)
{
try
{
    const auto reqbody = miscellaneous::unserializedJson(req.body);
    if(!reqbody.isMember("pageSize") || !reqbody.isMember("pageIndex"))
        throw std::invalid_argument("invalid params");
    const string pageSize = reqbody["pageSize"].asString();
    const string pageIndex = reqbody["pageIndex"].asString();

    const string level = reqbody.isMember("level") ? reqbody["level"].asString() : "";
    const string deviceName = reqbody.isMember("deviceName") ? reqbody["deviceName"].asString() : "";
    const string beginTime = reqbody.isMember("beginTime") ? reqbody["beginTime"].asString() : "";
    const string endTime = reqbody.isMember("endTime") ? reqbody["endTime"].asString() : "";
    const string actionType = reqbody.isMember("actionType") ? reqbody["actionType"].asString() : "";
    const string processed = reqbody.isMember("processed") ? reqbody["processed"].asString() : "";

    const auto getResult = getRecord(pageSize, pageIndex, level,
                                    deviceName, beginTime, endTime, actionType, processed);
    Json::Value root(Json::arrayValue);
    for(const auto& item: getResult){
        Json::Value node;
        node["content"] = std::get<0>(item);
        node["level"] = std::get<1>(item);
        node["deviceName"] = std::get<2>(item);
        node["createTime"] = std::get<3>(item);
        node["actionType"] = std::get<4>(item);
        node["processed"] = std::get<5>(item);
        root.append(node);
    }
    
    // 成功响应
    Json::Value repJson;
    // repJson["data"] = miscellaneous::unserializedJson(respondContent);
    repJson["data"] = root;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e){
    Json::Value msg;
    msg["data"] = Json::Value(Json::arrayValue);
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void HistoryWarning::requestCallbackGetPageInfo(const httplib::Request &req, httplib::Response &res)
{
try
{
    const auto reqbody = miscellaneous::unserializedJson(req.body);
    if(!reqbody.isMember("pageSize"))
        throw std::invalid_argument("invalid params");
    const string pageSize = reqbody["pageSize"].asString();

    const string level = reqbody.isMember("level") ? reqbody["level"].asString() : "";
    const string deviceName = reqbody.isMember("deviceName") ? reqbody["deviceName"].asString() : "";
    const string beginTime = reqbody.isMember("beginTime") ? reqbody["beginTime"].asString() : "";
    const string endTime = reqbody.isMember("endTime") ? reqbody["endTime"].asString() : "";
    const string actionType = reqbody.isMember("actionType") ? reqbody["actionType"].asString() : "";
    const string processed = reqbody.isMember("processed") ? reqbody["processed"].asString() : "";

    const auto getResult = getRecordPageInfo(pageSize, level, deviceName, beginTime, endTime, actionType, processed);

    Json::Value root;
    root["totalCount"] = std::get<0>(getResult);
    root["pageNum"] = std::get<1>(getResult);
    root["pageSize"] = std::get<2>(getResult);

    // 成功响应
    Json::Value repJson;
    repJson["data"] = root;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e){
    Json::Value msg;
    msg["data"] = Json::Value(Json::objectValue);
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void HistoryWarning::createTable()
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Warning.sqlite" };
        sqlite::database WarningDb(filename);

        WarningDb << "CREATE TABLE IF NOT EXISTS WARN("
                        "ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "CONTENT TEXT, "
                        "LEVEL TEXT, "
                        "DEVICE_NAME TEXT, "
                        "CREATE_TIME TEXT, "
                        "ACTION TEXT, "
                        "PROCESSED TEXT);";
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
}

void HistoryWarning::insertRecord(const string content, const string level, const string deviceName, const string createTime, const string action, const string processed)
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Warning.sqlite" };
        sqlite::database WarningDb(filename);
        WarningDb << "INSERT INTO WARN ("
                        "CONTENT, "         // 告警内容
                        "LEVEL, "           // 告警级别
                        "DEVICE_NAME, "     // 所属设备名称
                        "CREATE_TIME, "     // 记录产生时间
                        "ACTION, "           // 动作
                        "PROCESSED) "       // 处理状态
                        "VALUES (?, ?, ?, ?, ?, ?);"
                    << content << level << deviceName << createTime << action << processed;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
}

BauStatusProtectStatus::BauStatusProtectStatus()
{
    bits_.emplace_back(0, "单体高压", ActionType::Unchange);
    bits_.emplace_back(1, "单体低压", ActionType::Unchange);
}

void BauStatusProtectStatus::flushBits(const uint32_t oldValue, const uint32_t newValue)
{
    const bitset<32> oldValueBitMap{ oldValue };
    const bitset<32> newValueBitMap{ newValue };

    for(auto& item : bits_){
        const int bitIndex = std::get<0>(item);
        const bool oldBit = oldValueBitMap[bitIndex];
        const bool newBit = newValueBitMap[bitIndex];

        if(oldBit == newBit){
            continue;// 保持默认值 ActionType::Unchange
        }
        if(oldBit && !newBit){// 产生告警
            std::get<2>(item) = ActionType::Enable;
        }
        else{// 告警消失
            std::get<2>(item) = ActionType::Disable;
        }
    }
}

void HistoryWarning::handleBauStatusSummary(const int branchIndex, const bau::BauStatusSummary& oldValue, const bau::BauStatusSummary& newValue)
{
    const string createTime = []{

        using std::chrono::system_clock;
        auto now = system_clock::now();
        auto now_c = system_clock::to_time_t(now);
        auto now_tm = localtime(&now_c);

        ostringstream oss;
        oss << put_time(now_tm, "%F %T");// => "%Y-%m-%d" + "%H:%M:%S"
        return oss.str();
    }();

    {
        BauStatusProtectStatus protectStatus;
        protectStatus.flushBits(oldValue.protectStatusL1, newValue.protectStatusL1);

        const string deviceName{ "BAU" + to_string(branchIndex) + '#' };
        const string level{ "1" };
        for(const auto& item: protectStatus.bits_){

            map<ActionType, string> actionMap{
                { ActionType::Enable, "enable" },
                { ActionType::Disable, "disable" },
                { ActionType::Unchange, "unchange" }
            };

            const ActionType action = std::get<2>(item);
            if(action == ActionType::Unchange){
                continue;// 不需要保存记录
            }
            const string content = std::get<1>(item);
            insertRecord(content, level, deviceName, createTime, actionMap[action], "no");
        }
    }
}

vector<HistoryWarning::Record> HistoryWarning::getRecord(const string pageSize, const string pageIndex,
                                                        const string level, const string deviceName,
                                                        const string beginTime, const string endTime,
                                                        const string action, const string processed) const
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Warning.sqlite" };
    sqlite::database WarnDb(filename);

    vector<Record> records;

    const string patternLevel = level + "%";
    const string patternDeviceName = deviceName + "%";
    const string patternBeginTime = beginTime.empty() ? "1900-01-01" : beginTime;
    const string patternEndTime = endTime.empty() ? "2050-01-01" : endTime;
    const string patternAction = action + "%";
    const string patternProcessed = processed + "%";

    const string patternLimit = pageSize;
    const string patternOffset = to_string(std::stoi(pageSize) * (std::stoi(pageIndex) + 1));

    // 查询
    WarnDb << "SELECT "
                "CONTENT, LEVEL, DEVICE_NAME, CREATE_TIME, ACTION, PROCESSED "
                "FROM WARN WHERE LEVEL LIKE ?"
                "AND DEVICE_NAME LIKE ? "
                "AND CREATE_TIME > ? "
                "AND CREATE_TIME < ? "
                "AND ACTION LIKE ? "
                "AND PROCESSED LIKE ? "
                "LIMIT ? OFFSET ?"
            << patternLevel << patternDeviceName
            << patternBeginTime << patternEndTime
            << patternAction << patternProcessed
            << patternLimit << patternOffset
            >> [&](string selectedContent, string selectedLevel,
                    string selectedDeviceName,
                    string selectedCreateTime, string selectedAction,
                    string selectedProcessed){
                        
                        records.emplace_back(selectedContent, selectedLevel, selectedDeviceName,
                                            selectedCreateTime, selectedAction, selectedProcessed);
                    };
    if(records.empty())
        throw std::runtime_error("record is empty");
    return records;
}                                                     

tuple<string, string, string> HistoryWarning::getRecordPageInfo(const string pageSize, const string level,
                                                        const string deviceName,
                                                        const string beginTime, const string endTime,
                                                        const string action, const string processed) const
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Warning.sqlite" };
    sqlite::database WarnDb(filename);

    vector<Record> records;

    const string patternLevel = level + "%";
    const string patternDeviceName = deviceName + "%";
    const string patternBeginTime = beginTime.empty() ? "1900-01-01" : beginTime;
    const string patternEndTime = endTime.empty() ? "2050-01-01" : endTime;
    const string patternAction = action + "%";
    const string patternProcessed = processed + "%";

    // 查询
    int recordCounts;
    WarnDb << "SELECT COUNT(*) "
                "FROM WARN WHERE LEVEL LIKE ?"
                "AND DEVICE_NAME LIKE ? "
                "AND CREATE_TIME > ? "
                "AND CREATE_TIME < ? "
                "AND ACTION LIKE ? "
                "AND PROCESSED LIKE ?;"
            << patternLevel << patternDeviceName
            << patternBeginTime << patternEndTime
            << patternAction << patternProcessed
            >> recordCounts;
    
    int pageNum = recordCounts / stoi(pageSize);
    if(recordCounts > 0){
        if(pageNum == 0){
            pageNum = 1;
        }
        else if(recordCounts % stoi(pageSize) > 0){
            pageNum += 1;
        }
    }
    return { to_string(recordCounts), to_string(pageNum), pageSize };
}                                                     
