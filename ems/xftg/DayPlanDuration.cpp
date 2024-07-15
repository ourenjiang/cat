#include "DayPlanDuration.h"
#include "boost/assert.hpp"
#include <filesystem>
#include "sqlite_modern_cpp.h"
#include "json/json.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/interface/UserManager.h"
#include "utils/AuthException.h"
#include "ems/base/UserManager.h"
#include "utils/datetime.h"
#include "utils/jsonWrapper.h"
#include "utils/StreamWrapper.h"
#include "ems/base/Database.h"
#include "ems/base/OperationRecord.h"

using namespace ems;
using namespace ems::xftg;

void DayPlanDuration::createTable()
{
try
{
    auto XftgDb = base::Database::open("/Xftg.sqlite");
    XftgDb << "CREATE TABLE IF NOT EXISTS XFTG_DAYPLAN_DURATION("
            "NAME TEXT,"
            "DURATION_NAME TEXT,"
            "DURATION_BEGIN TEXT,"
            "DURATION_END TEXT,"
            "CONTROL_TYPE TEXT,"
            "TARGET_SOC TEXT, "
            "TARGET_POWER TEXT, "
            "PRIMARY KEY(NAME, DURATION_NAME))";
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void DayPlanDuration::insertIntoDefaultRecord()
{
try
{
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    int recordCount{0};
    XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_DURATION;" >> recordCount;
    if(recordCount > 0) return;

    /** 这里模拟两充一放的时段组合，共3条记录 */
    {
        XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                    "NAME, DURATION_NAME, DURATION_BEGIN, DURATION_END, "
                    "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) VALUES (?, ?, ?, ?, ?, ?, ?);"
                    << "dayPlanDuration1" << "duration1" << "00:00" << "08:00" << "charge" << "100%" << "10kW";
        XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                    "NAME, DURATION_NAME, DURATION_BEGIN, DURATION_END, "
                    "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) VALUES (?, ?, ?, ?, ?, ?, ?);"
                    << "dayPlanDuration1" << "duration2" << "08:00" << "12:00" << "charge" << "100%" << "10kW";
        XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                    "NAME, DURATION_NAME, DURATION_BEGIN, DURATION_END, "
                    "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) VALUES (?, ?, ?, ?, ?, ?, ?);"
                    << "dayPlanDuration1" << "duration3" << "12:00" << "~" << "charge" << "100%" << "10kW";
    }

    /** 这里模拟两充两放的时段组合，共4条记录 */
    {
        XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                    "NAME, DURATION_NAME, DURATION_BEGIN, DURATION_END, "
                    "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) VALUES (?, ?, ?, ?, ?, ?, ?);"
                    << "dayPlanDuration2" << "duration1" << "00:00" << "08:00" << "charge" << "100%" << "10kW";
        XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                    "NAME, DURATION_NAME, DURATION_BEGIN, DURATION_END, "
                    "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) VALUES (?, ?, ?, ?, ?, ?, ?);"
                    << "dayPlanDuration2" << "duration2" << "08:00" << "12:00" << "discharge" << "100%" << "10kW";
        XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                    "NAME, DURATION_NAME, DURATION_BEGIN, DURATION_END, "
                    "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) VALUES (?, ?, ?, ?, ?, ?, ?);"
                    << "dayPlanDuration2" << "duration3" << "12:00" << "19:00" << "charge" << "100%" << "10kW";
        XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                    "NAME, DURATION_NAME, DURATION_BEGIN, DURATION_END, "
                    "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) VALUES (?, ?, ?, ?, ?, ?, ?);"
                    << "dayPlanDuration2" << "duration4" << "19:00" << "~" << "discharge" << "100%" << "10kW";
    }
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

pair<string, vector<DayPlanDuration::Record>> DayPlanDuration::parseRecordFromRequestBody(const Json::Value& root)
{
    if(!root.isMember("name") || root["name"].empty())// 存在且不为空
        throw std::runtime_error("request params err");
    const string dayPlanName = root["name"].asString();

    if(!root.isMember("durationList"))// 存在且不为空
        throw std::runtime_error("request params err");
    const auto& durationList = root["durationList"]; 
    if(!durationList.isArray() || durationList.empty())// 数组且不为空
        throw std::runtime_error("request params err");
    
    vector<DayPlanDuration::Record> validRecords;
    // 解析内容并做格式校验
    for(const auto& item: durationList){
        if(!item.isMember("durationName"))
            throw std::runtime_error("request params err");
        const string durationName = item["durationName"].asString();
        
        if(!item.isMember("durationBegin"))
            throw std::runtime_error("request params err");
        const string durationBegin = item["durationBegin"].asString();
        if(!stream_wrapper::matchTimeHHMM(durationBegin))
            throw std::runtime_error("request params err");
        
        if(!item.isMember("durationEnd"))
            throw std::runtime_error("request params err");
        const string durationEnd = item["durationEnd"].asString();
        if(!stream_wrapper::matchTimeHHMM(durationEnd))
            throw std::runtime_error("request params err");

        if(!item.isMember("controlType"))
            throw std::runtime_error("request params err");
        const string controlType = item["controlType"].asString();
        if(controlType != "charge" && controlType != "discharge" && controlType != "standby")
            throw std::runtime_error("request params err");
        
        if(!item.isMember("targetSoc"))
            throw std::runtime_error("request params err");
        const string targetSoc = item["targetSoc"].asString();
        if(!std::regex_match(targetSoc, std::regex(R"((\d+)%)")))
            throw std::runtime_error("request params err");
        
        if(!item.isMember("targetPower"))
            throw std::runtime_error("request params err");
        const string targetPower = item["targetPower"].asString();
        if(!std::regex_match(targetPower, std::regex(R"((\d+)kW)")))
            throw std::runtime_error("request params err");
        validRecords.emplace_back(durationName,
            durationBegin, durationEnd, controlType, targetSoc, targetPower);
    }
    return { dayPlanName, validRecords };
}

void DayPlanDuration::requestCallbackPost(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const auto usernameAuth = base::UserManager::doAuth(reqbody);

    /** 解析业务参数 */
    const auto& [dayPlanName, durationList] = parseRecordFromRequestBody(reqbody);

    // 打开数据库
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    // 检查记录是否已存在
    int recordCount{ 0 };
    XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_DURATION WHERE NAME = ?;" << dayPlanName >> recordCount;
    if(recordCount > 0) throw AuthException("记录已存在", usernameAuth);

    for(const auto& [durationName, durationBegin, durationEnd, controlType, targetSoc, targetPower]: durationList){
        XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                    "NAME, DURATION_NAME, DURATION_BEGIN, DURATION_END, CONTROL_TYPE, TARGET_SOC, TARGET_POWER) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?);"
                << dayPlanName << durationName << durationBegin << durationEnd << controlType << targetSoc << targetPower;
    }

    // 通知站点重新加载数据
    cmdDealer->send(zmq::message_t(string("Strategy0Set")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("InterfaceCmd")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("DayPlanDuration")), zmq::send_flags::none);

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
    base::OperationRecord::insertRecord("success", "添加削峰填谷日计划", "参数设置", usernameAuth);

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

void DayPlanDuration::requestCallbackGet(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    if(!req.has_param("name"))
        throw std::runtime_error("request params err");
    const string dayPlanName = req.get_param_value("name");

    Json::Value durationList;
    for(const auto& item: getRecord(dayPlanName)){
        Json::Value record;
        record["name"] = dayPlanName;
        record["durationName"] = std::get<0>(item);
        record["durationBegin"] = std::get<1>(item);
        record["durationEnd"] = std::get<2>(item);
        record["controlType"] = std::get<3>(item);
        record["targetSoc"] = std::get<4>(item);
        record["targetPower"] = std::get<5>(item);
        durationList.append(record);
    }

    Json::Value respondContent;
    respondContent["data"] = durationList;
    respondContent["errcode"] = 0;
    respondContent["errmsg"] = "success";
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){
    Json::Value msg;
    msg["data"] = Json::Value(Json::arrayValue);
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void DayPlanDuration::requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    // 查询
    vector<string> namelist;
    XftgDb << "SELECT "
                "DISTINCT NAME "// 注意去重
                "FROM XFTG_DAYPLAN_DURATION;"
            >> [&](string name){
                    namelist.push_back(name);
                };
    if(namelist.empty())
        throw std::runtime_error("record is empty");

    Json::Value data;
    for(const auto& item: namelist){
        data.append(item);
    }

    Json::Value repJson;
    repJson["data"] = data;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e)
{
    Json::Value respondmsg;
    respondmsg["data"] = Json::Value(Json::arrayValue);
    respondmsg["errcode"] = -2;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

void DayPlanDuration::requestCallbackDelete(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const auto usernameAuth = base::UserManager::doAuth(reqbody);
    const string dayPlanName = reqbody["name"].asString();

    //操作数据库
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    {
        // 检查记录是否存在
        int recordCount{0};
        XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_DURATION WHERE NAME = ?;"
            << dayPlanName >> recordCount;
        if(recordCount == 0)
            throw std::runtime_error("记录不存在");
    }

    {
        // 检查是否有周计划记录绑定了该保护参数；
        int recordCount{0};
        XftgDb << "SELECT COUNT(*) FROM XFTG_WEEK_PLAN WHERE DAYPLAN_DURATION_NAME = ?;"
            << dayPlanName >> recordCount;
        if(recordCount > 0)
            throw std::runtime_error("记录在周计划中被占用");
    }
    // 执行删除
    XftgDb << "DELETE FROM XFTG_DAYPLAN_DURATION WHERE NAME = ?;" << dayPlanName;

    // 通知站点重新加载数据
    cmdDealer->send(zmq::message_t(string("Strategy0Set")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("InterfaceCmd")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("DayPlanDuration")), zmq::send_flags::none);

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
    base::OperationRecord::insertRecord("success", "删除削峰填谷日计划", "参数设置", usernameAuth);

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

void DayPlanDuration::requestCallbackPut(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const auto usernameAuth = base::UserManager::doAuth(reqbody);

    /** 解析业务参数 */
    if(!reqbody.isMember("name") || !reqbody.isMember("durationList"))
        throw std::runtime_error("request params err");
    const auto& durationList = reqbody["durationList"];
    if(durationList.empty() || !durationList.isArray())
        throw std::runtime_error("request params err");
    const string dayPlanName = reqbody["name"].asString();

    // 打开数据库
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    // 判断目标记录是否存在
    int recordCount{0};
    XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_DURATION WHERE NAME = ?;"
        << dayPlanName  >> recordCount;
    if(recordCount == 0)
        throw std::invalid_argument("record not exist");
    
    for(const auto& item: durationList){
        if(!item.isMember("durationName")
            || !item.isMember("durationBegin") || !item.isMember("durationEnd")
            || !item.isMember("controlType") || !item.isMember("targetSoc")
            || !item.isMember("targetPower"))
            throw std::invalid_argument("invalid params");

        // 对于时段项, 存在则修改，不存在则添加
        int durationRecordCount{0};
        XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_DURATION WHERE NAME = ? AND DURATION_NAME = ?;"
            << dayPlanName << item["durationName"].asString() >> durationRecordCount;
        if(durationRecordCount > 0){
            XftgDb << "UPDATE XFTG_DAYPLAN_DURATION SET "
                        "DURATION_BEGIN = ?, DURATION_END = ?, "
                        "CONTROL_TYPE = ?, TARGET_SOC = ?, TARGET_POWER = ? "
                        "WHERE NAME = ? AND DURATION_NAME = ?;"
                    << item["durationBegin"].asString() << item["durationEnd"].asString()
                    << item["controlType"].asString() << item["targetSoc"].asString()
                    << item["targetPower"].asString()
                    << dayPlanName << item["durationName"].asString();
        }
        else
        {
            XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                    "NAME, DURATION_NAME, "
                    "DURATION_BEGIN, DURATION_END, "
                    "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) "
                    "VALUES (?, ?, ?, ?, ?, ?, ?);"
                    << dayPlanName << item["durationName"].asString()
                    << item["durationBegin"].asString() << item["durationEnd"].asString()
                    << item["controlType"].asString() << item["targetSoc"].asString()
                    << item["targetPower"].asString();
        }
    }

    // 通知站点重新加载数据
    cmdDealer->send(zmq::message_t(string("Strategy0Set")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("InterfaceCmd")), zmq::send_flags::sndmore);
    cmdDealer->send(zmq::message_t(string("DayPlanDuration")), zmq::send_flags::none);

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
    base::OperationRecord::insertRecord("success", "修改削峰填谷日计划", "参数设置", usernameAuth);

    // 成功响应
    Json::Value repJson;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e)
{
    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

vector<DayPlanDuration::Record> DayPlanDuration::getRecord(const string& name)
{
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    // 准备查询结果
    vector<Record> records;

    // 查询
    XftgDb << "SELECT "
            "DURATION_NAME, "
            "DURATION_BEGIN, "
            "DURATION_END, "
            "CONTROL_TYPE, "
            "TARGET_SOC, TARGET_POWER "
            "FROM XFTG_DAYPLAN_DURATION "
            "WHERE NAME = ?;"
            << name
            >> [&](string durationName,
                    string durationBegin, string durationEnd,
                    string controlType, string targetSoc, string targetPower){
                        
                        records.emplace_back(durationName, durationBegin, durationEnd,
                                                controlType, targetSoc, targetPower);
                    };
    
    if(records.empty())
        throw std::runtime_error("record is empty");
    return records;
}

std::optional<map<string, vector<DayPlanDuration::Record>>> DayPlanDuration::getAllRecord()
{
try
{
    auto XftgDb = base::Database::open("/Xftg.sqlite");

    // 准备查询结果
    map<string, vector<Record>> recordsMap;

    // 查询
    XftgDb << "SELECT "
            "NAME, "
            "DURATION_NAME, "
            "DURATION_BEGIN, "
            "DURATION_END, "
            "CONTROL_TYPE, "
            "TARGET_SOC, TARGET_POWER "
            "FROM XFTG_DAYPLAN_DURATION;"
            >> [&](string name, string durationName,
                    string durationBegin, string durationEnd,
                    string controlType, string targetSoc, string targetPower){
                        
                        vector<Record>& records = recordsMap[name];
                        records.emplace_back(durationName, durationBegin, durationEnd,
                                                controlType, targetSoc, targetPower);
                    };
    
    if(!recordsMap.empty())
        return std::optional<map<string, vector<Record>>>(recordsMap);
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
return {};
}
