#include "DayPlan.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "sqlite_modern_cpp.h"
#include "json/json.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "ems/station/OperationRecord.h"
#include "ems/station/UserManager.h"
#include "ems/station/AuthException.h"

using namespace ems;
using namespace ems::electricity_price;

DayPlan::DayPlan()
{
    createTable();
    insertIntoDefaultRecord();
    registerHttpInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void DayPlan::registerHttpInterfaces()
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    auto& serv = utils::getHttpServerSingleton();
    serv.Post("/xftg/electricityPrice/dayPlan", httplib::Server::Handler(bind(&DayPlan::requestCallbackPost, this, _1, _2)));
    serv.Get("/xftg/electricityPrice/dayPlan", httplib::Server::Handler(bind(&DayPlan::requestCallbackGet, this, _1, _2)));
    serv.Get("/xftg/electricityPrice/dayPlan/namelist", httplib::Server::Handler(bind(&DayPlan::requestCallbackGetNameList, this, _1, _2)));
    serv.Delete("/xftg/electricityPrice/dayPlan", httplib::Server::Handler(bind(&DayPlan::requestCallbackDelete, this, _1, _2)));
    serv.Put("/xftg/electricityPrice/dayPlan", httplib::Server::Handler(bind(&DayPlan::requestCallbackPut, this, _1, _2)));
}

vector<byte> DayPlan::respondCallbackPost(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 这里需要从数据库重新加载这部分记录.

    // 返回结果
    return miscellaneous::convertStringToBytes("success");
}

vector<byte> DayPlan::respondCallbackPut(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 需要重新加载

    // 返回结果
    return miscellaneous::convertStringToBytes("success");
}

vector<byte> DayPlan::respondCallbackDelete(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    string name;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), name);

    // 重新加载数据库
    return miscellaneous::convertStringToBytes("success");
}

void DayPlan::requestCallbackPost(const httplib::Request &req, httplib::Response &res)
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

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    // 检查记录是否已存在
    int recordCount{ 0 };
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_DAYPLAN WHERE NAME = ?;"
        << dayPlanName >> recordCount;
    if(recordCount > 0)
        throw AuthException("记录已存在", auth["username"].asString());

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

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("ElectricityPriceDayPlanPost");
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

void DayPlan::requestCallbackGet(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("name"))
        throw std::runtime_error("request params err");
    const string durationName = req.get_param_value("name");

    RecordList recordList;
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

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

void DayPlan::requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res)
{
try
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

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

void DayPlan::requestCallbackDelete(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("name"))
        throw std::runtime_error("request params err");
    const string dayPlanName = req.get_param_value("name");

    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

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

    // 准备请求消息
    string publishContent;
    auto serializedMsg = msgpackWrapper::pack(dayPlanName);
    const string topic = miscellaneous::createFixedSizeString("ElectricityPriceDayPlanDelete");
    std::copy(topic.data(), topic.data() + topic.size(), std::back_inserter(publishContent));
    std::copy(serializedMsg.data(), serializedMsg.data() + serializedMsg.size(), std::back_inserter(publishContent));

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

void DayPlan::requestCallbackPut(const httplib::Request &req, httplib::Response &res)
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

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    // 检查记录是否已存在
    int recordCount{ 0 };
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_DAYPLAN WHERE NAME = ?;"
        << dayPlanName >> recordCount;
    if(recordCount == 0)
        throw AuthException("记录不存在", auth["username"].asString());

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
    
    // 准备请求参数
    auto serializedMsg = msgpackWrapper::pack(dayPlanName);

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("ElectricityPriceDayPlanPut");
    std::copy(topic.data(), topic.data() + topic.size(), std::back_inserter(publishContent));
    std::copy(serializedMsg.data(), serializedMsg.data() + serializedMsg.size(), std::back_inserter(publishContent));

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

std::string DayPlan::createTable() const
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
        ElectricityPriceDb << "CREATE TABLE IF NOT EXISTS ELECTRICITY_PRICE_DAYPLAN("
                              "NAME TEXT, "
                              "DURATION_NAME TEXT, "
                              "DURATION_TYPE TEXT, "
                              "DURATION_BEGIN TEXT, "
                              "DURATION_END TEXT, "
                              "PRIMARY KEY(NAME, DURATION_NAME))";
        return filename;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
}

bool DayPlan::insertIntoDefaultRecord() const
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/ElectricityPrice.sqlite" };
        sqlite::database ElectricityPriceDb(filename);

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
        return true;
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
    return {};
}

void DayPlan::insertRecord(const string& name,
                            const string& durationName, const string& durationType,
                            const string& durationBegin, const string& durationEnd) const
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_DAYPLAN ("
                            "NAME, "
                            "DURATION_NAME, DURATION_TYPE, "
                            "DURATION_BEGIN, DURATION_END) "
                            "VALUES (?, ?, ?, ?, ?);"
                        << name
                        << durationName << durationType
                        << durationBegin << durationEnd;
}
