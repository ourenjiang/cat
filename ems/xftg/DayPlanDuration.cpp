#include "DayPlanDuration.h"
#include "boost/assert.hpp"
#include <filesystem>
#include "sqlite_modern_cpp.h"
#include "json/json.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/station/UserManager.h"
#include "ems/station/AuthException.h"

using namespace ems::xftg;

DayPlanDuration::DayPlanDuration()
{
    createTable();
    insertIntoDefaultRecord();
    registerHttpInterfaces();

    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void DayPlanDuration::registerHttpInterfaces()
{
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Post("/strategy/xftg/dayPlan", httplib::Server::Handler(bind(&DayPlanDuration::requestCallbackPost, this, _1, _2)));
    serv.Get("/strategy/xftg/dayPlan", httplib::Server::Handler(bind(&DayPlanDuration::requestCallbackGet, this, _1, _2)));
    serv.Get("/strategy/xftg/dayPlan/namelist", httplib::Server::Handler(bind(&DayPlanDuration::requestCallbackGetNameList, this, _1, _2)));
    serv.Delete("/strategy/xftg/dayPlan", httplib::Server::Handler(bind(&DayPlanDuration::requestCallbackDelete, this, _1, _2)));
    serv.Put("/strategy/xftg/dayPlan", httplib::Server::Handler(bind(&DayPlanDuration::requestCallbackPut, this, _1, _2)));
}

void DayPlanDuration::requestCallbackPost(const httplib::Request &req, httplib::Response &res)
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
    const string dayPlanName = reqbody["name"].asString();

    // 打开数据库
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    // 检查记录是否已存在
    int recordCount{ 0 };
    XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_DURATION WHERE NAME = ?;"
        << dayPlanName >> recordCount;
    if(recordCount > 0)
        throw AuthException("记录已存在", auth["username"].asString());

    for(const auto& item: durationList){

        if(!item.isMember("durationName")
            || !item.isMember("durationBegin")
            || !item.isMember("durationEnd")
            || !item.isMember("controlType")
            || !item.isMember("targetSoc")
            || !item.isMember("targetPower")){
                throw std::runtime_error("request params err");
            }
        
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

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("XftgDayPlanDurationPost");
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

void DayPlanDuration::requestCallbackGet(const httplib::Request &req, httplib::Response &res)
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

void DayPlanDuration::requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res)
{
try
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

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

void DayPlanDuration::requestCallbackDelete(const httplib::Request &req, httplib::Response &res)
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
    const string dayPlanName = reqbody["name"].asString();

    //操作数据库
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

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

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("XftgDayPlanDurationDelete");
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

vector<byte> DayPlanDuration::respondCallbackDelete(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 返回结果
    return miscellaneous::convertStringToBytes("success");
}

vector<byte> DayPlanDuration::respondCallbackPost(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 重载数据库

    // 返回结果
    return miscellaneous::convertStringToBytes("success");
}

vector<byte> DayPlanDuration::respondCallbackPut(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{

    // // 重新加载数据库表缓存
    // auto getResult = getRecord(name);// TODO!!!
    // if(getResult.has_value()){

    //     // 所有依赖了此模板的分支，都将得到更新；
    //     for(auto& item : stationInfo->branchList){

    //         auto& branchInfo = item.second;
    //         auto& xftgStrategy = branchInfo->xftgStrategy;
    //         const string dayPlanName = xftgStrategy.getDayPlanName();
    //         if(dayPlanName == name){

    //             auto& rawValue = getResult.value().second;
    //             std::vector<xftg::DurationInfo> durations;
    //             std::transform(rawValue.begin(), rawValue.end(), std::back_inserter(durations),
    //                 [&](const Record& record){
    //                     xftg::DurationInfo info;
    //                     info.durationName = std::get<0>(record);
    //                     info.durationBegin = std::get<1>(record);
    //                     info.durationEnd = std::get<2>(record);
    //                     info.controlType = std::get<3>(record);
    //                     info.targetSoc = std::stoi(std::get<4>(record));
    //                     info.targetPower = std::stod(std::get<5>(record));
    //                     return info;
    //             });

    //             xftgStrategy.setDurationInfo(make_pair(name, durations));
    //         }
    //     }
    // }

    // 返回结果
    return miscellaneous::convertStringToBytes("success");
}

optional<string> DayPlanDuration::request(const string& reqmsg)
{
    const vector<byte> sendmsg(reinterpret_cast<const byte*>(reqmsg.data()),
                                reinterpret_cast<const byte*>(reqmsg.data()) + reqmsg.size());
    const bool sendResult = requester_->send(sendmsg);
    if(!sendResult){
        cout << "send err" << endl;
        return {};
    }

    // 接收
    const auto recvResult = requester_->recv();
    if(!recvResult.has_value()){
        cout << "recv failed" << endl;
        return {};
    }
    const auto recvmsg = recvResult.value();
    return string(reinterpret_cast<const char*>(recvmsg.data()),
                    reinterpret_cast<const char*>(recvmsg.data()) + recvmsg.size());
}

void DayPlanDuration::requestCallbackPut(const httplib::Request &req, httplib::Response &res)
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
    const string dayPlanName = reqbody["name"].asString();

    // 打开数据库
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    for(const auto& item: durationList){
        if(!item.isMember("durationName")
            || !item.isMember("durationBegin") || !item.isMember("durationEnd")
            || !item.isMember("controlType") || !item.isMember("targetSoc")
            || !item.isMember("targetPower"))
            throw std::invalid_argument("invalid params");

        // 存在则修改，不存在则添加
        int recordCount{0};
        XftgDb << "SELECT COUNT(*) FROM XFTG_DAYPLAN_DURATION WHERE NAME = ? AND DURATION_NAME = ?;"
            << dayPlanName << item["durationName"].asString() >> recordCount;
        if(recordCount > 0){
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

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("XftgDayPlanDurationPut");
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
catch(const std::invalid_argument& e)
{
    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

string DayPlanDuration::createTable()
{
    string finalResult;

    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Xftg.sqlite" };
        sqlite::database XftgDb(filename);

        XftgDb << "CREATE TABLE IF NOT EXISTS XFTG_DAYPLAN_DURATION("
                              "NAME TEXT,"
                              "DURATION_NAME TEXT,"
                              "DURATION_BEGIN TEXT,"
                              "DURATION_END TEXT,"
                              "CONTROL_TYPE TEXT,"
                              "TARGET_SOC TEXT, "
                              "TARGET_POWER TEXT, "
                              "PRIMARY KEY(NAME, DURATION_NAME))";
        finalResult =  filename;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return string();
    }
    return finalResult;
}

bool DayPlanDuration::insertIntoDefaultRecord()
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Xftg.sqlite" };
        sqlite::database XftgDb(filename);

        XftgDb << "DELETE FROM XFTG_DAYPLAN_DURATION;";// clear old records

        /** 这里模拟两充一放的时段组合，共3条记录 */
        {
            XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                        "NAME, DURATION_NAME, "
                        "DURATION_BEGIN, DURATION_END, "
                        "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?);"
                        << "dayPlanDuration1" << "duration1"
                        << "00:00" << "08:00"
                        << "charge" << "100%" << "10kW";
                        
            XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                        "NAME, DURATION_NAME, "
                        "DURATION_BEGIN, DURATION_END, "
                        "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?);"
                        << "dayPlanDuration1" << "duration2"
                        << "08:00" << "12:00"
                        << "charge" << "100%" << "10kW";

            XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                        "NAME, DURATION_NAME, "
                        "DURATION_BEGIN, DURATION_END, "
                        "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?);"
                        << "dayPlanDuration1" << "duration3"
                        << "12:00" << "~"
                        << "charge" << "100%" << "10kW";
        }

        /** 这里模拟两充两放的时段组合，共4条记录 */
        {
            XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                        "NAME, DURATION_NAME, "
                        "DURATION_BEGIN, DURATION_END, "
                        "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?);"
                        << "dayPlanDuration2" << "duration1"
                        << "00:00" << "08:00"
                        << "charge" << "100%" << "10kW";
                        
            XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                        "NAME, DURATION_NAME, "
                        "DURATION_BEGIN, DURATION_END, "
                        "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?);"
                        << "dayPlanDuration2" << "duration2"
                        << "08:00" << "12:00"
                        << "discharge" << "100%" << "10kW";

            XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                        "NAME, DURATION_NAME, "
                        "DURATION_BEGIN, DURATION_END, "
                        "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?);"
                        << "dayPlanDuration2" << "duration3"
                        << "12:00" << "19:00"
                        << "charge" << "100%" << "10kW";
            XftgDb << "INSERT INTO XFTG_DAYPLAN_DURATION ("
                        "NAME, DURATION_NAME, "
                        "DURATION_BEGIN, DURATION_END, "
                        "CONTROL_TYPE, TARGET_SOC, TARGET_POWER) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?);"
                        << "dayPlanDuration2" << "duration4"
                        << "19:00" << "~"
                        << "discharge" << "100%" << "10kW";
        }
        return true;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
}

vector<DayPlanDuration::Record> DayPlanDuration::getRecord(const string& name)
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

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
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Xftg.sqlite" };
        sqlite::database XftgDb(filename);

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
