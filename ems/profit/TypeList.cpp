#include "TypeList.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "sqlite_modern_cpp.h"
#include "json/json.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/station/OperationRecord.h"
#include "ems/station/AuthException.h"
#include "ems/station/UserManager.h"

using namespace ems;
using namespace ems::electricity_price;

TypeList::TypeList()
{
    createTable();
    insertIntoDefaultRecord();
    registerHttpInterfaces();

    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void TypeList::registerHttpInterfaces()
{
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Post("/xftg/electricityPrice/typeList", httplib::Server::Handler(bind(&TypeList::requestCallbackPost, this, _1, _2)));
    serv.Put("/xftg/electricityPrice/typeList", httplib::Server::Handler(bind(&TypeList::requestCallbackPut, this, _1, _2)));
    serv.Get("/xftg/electricityPrice/typeList", httplib::Server::Handler(bind(&TypeList::requestCallbackGet, this, _1, _2)));
    serv.Get("/xftg/electricityPrice/typeList/namelist", httplib::Server::Handler(bind(&TypeList::requestCallbackGetNameList, this, _1, _2)));
    serv.Delete("/xftg/electricityPrice/typeList", httplib::Server::Handler(bind(&TypeList::requestCallbackDelete, this, _1, _2)));
}

void TypeList::requestCallbackPost(const httplib::Request &req, httplib::Response &res)
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
        || !reqbody.isMember("jianInput") || !reqbody.isMember("jianOutput")
        || !reqbody.isMember("fengInput") || !reqbody.isMember("fengOutput")
        || !reqbody.isMember("pingInput") || !reqbody.isMember("pingOutput")
        || !reqbody.isMember("guInput") || !reqbody.isMember("guOutput")){
        throw std::runtime_error("request params err");
    }
    const string typeListName = reqbody["name"].asString();
    const string jianInput = reqbody["jianInput"].asString();
    const string jianOutput = reqbody["jianOutput"].asString();
    const string fengInput = reqbody["fengInput"].asString();
    const string fengOutput = reqbody["fengOutput"].asString();
    const string pingInput = reqbody["pingInput"].asString();
    const string pingOutput = reqbody["pingOutput"].asString();
    const string guInput = reqbody["guInput"].asString();
    const string guOutput = reqbody["guOutput"].asString();

    // 操作数据库
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    // 检查记录是否已存在
    int typeListRecordCount{ 0 };
    ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_TYPELIST WHERE NAME = ?;"
        << typeListName >> typeListRecordCount;
    if(typeListRecordCount > 0)
        throw AuthException("电价类型已存在", usernameAuth);

    ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_TYPELIST ("
                            "NAME, "
                            "JIAN_INPUT, JIAN_OUTPUT, "
                            "FENG_INPUT, FENG_OUTPUT, "
                            "PING_INPUT, PING_OUTPUT, "
                            "GU_INPUT, GU_OUTPUT) "
                            "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);"
                        << typeListName
                        << jianInput << jianOutput << fengInput << fengOutput
                        << pingInput << pingOutput << guInput << guOutput;

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("ElectricityPriceTypeListPost");
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

vector<byte> TypeList::respondCallbackPost(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 重载数据库

    // 返回结果
    return miscellaneous::convertStringToBytes("success");
}

vector<byte> TypeList::respondCallbackPut(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 这里需要从数据库重新加载这部分记录.

    // 返回结果
    return miscellaneous::convertStringToBytes("success");
}

void TypeList::requestCallbackGet(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("name"))
        throw std::runtime_error("request params err");
    const string typeListName = req.get_param_value("name");

    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    std::optional<Record> record;
    // 查询
    ElectricityPriceDb << "SELECT "
                            "JIAN_INPUT, JIAN_OUTPUT, "
                            "FENG_INPUT, FENG_OUTPUT, "
                            "PING_INPUT, PING_OUTPUT, "
                            "GU_INPUT, GU_OUTPUT "
                            "FROM ELECTRICITY_PRICE_TYPELIST "
                            "WHERE NAME = ?;"
                        << typeListName
    >> [&](string jianInput_, string jianOutput_,
            string fengInput_, string fengOutput_,
            string pingInput_, string pingOutput_,
            string guInput_, string guOutput_){
                
                record.emplace(jianInput_, jianOutput_,
                                fengInput_, fengOutput_,
                                pingInput_, pingOutput_,
                                guInput_, guOutput_
                                );
            };
    if(!record.has_value())
        throw std::runtime_error("record not exists");

    Json::Value data(Json::objectValue);
    data["jianInput"] = std::get<0>(record.value());
    data["jianOutput"] = std::get<1>(record.value());
    data["fengInput"] = std::get<2>(record.value());
    data["fengOutput"] = std::get<3>(record.value());
    data["pingInput"] = std::get<4>(record.value());
    data["pingOutput"] = std::get<5>(record.value());
    data["guInput"] = std::get<6>(record.value());
    data["guOutput"] = std::get<7>(record.value());

    Json::Value respondContent;
    respondContent["data"] = data;
    respondContent["errcode"] = 0;
    respondContent["errmsg"] = "success";
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){
    Json::Value msg;
    msg["data"] = Json::Value(Json::objectValue);
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

void TypeList::requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res)
{
try
{
    // 操作数据库
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    // 准备查询结果
    NameList namelist;

    // 查询
    ElectricityPriceDb << "SELECT NAME FROM ELECTRICITY_PRICE_TYPELIST;"
                        >> [&](const string name){
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

vector<byte> TypeList::respondCallbackDelete(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    // 加载数据库

    // 返回结果
    const string result{ "success" };
    return { reinterpret_cast<const byte*>(result.data()),
            reinterpret_cast<const byte*>(result.data()) + result.size() };
}

void TypeList::requestCallbackDelete(const httplib::Request &req, httplib::Response &res)
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
    const string typeListName = auth["name"].asString();
    
    // 操作数据库
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    {
        // 检查记录是否存在
        int recordCount{0};
        ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_TYPELIST WHERE NAME = ?;"
            << typeListName >> recordCount;
        if(recordCount == 0)
            throw std::runtime_error("record not exists");
    }

    {
        // 检查是否有月计划记录绑定了该记录；
        int recordCount{0};
        ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_MONTHPLAN WHERE TYPELIST_NAME = ?;"
            << typeListName >> recordCount;
        if(recordCount > 0)
            throw std::runtime_error("record have used in other month plan");
    }

    // 执行删除
    ElectricityPriceDb << "DELETE FROM ELECTRICITY_PRICE_TYPELIST WHERE NAME = ?;"
                        << typeListName;

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("ElectricityPriceTypeListDelete");
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
    // 失败响应
    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

void TypeList::requestCallbackPut(const httplib::Request &req, httplib::Response &res)
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
        || !reqbody.isMember("jianInput") || !reqbody.isMember("jianOutput")
        || !reqbody.isMember("fengInput") || !reqbody.isMember("fengOutput")
        || !reqbody.isMember("pingInput") || !reqbody.isMember("pingOutput")
        || !reqbody.isMember("guInput") || !reqbody.isMember("guOutput")){
        throw std::runtime_error("request params err");
    }
    const string typeListName = reqbody["name"].asString();
    const string jianInput = reqbody["jianInput"].asString();
    const string jianOutput = reqbody["jianOutput"].asString();
    const string fengInput = reqbody["fengInput"].asString();
    const string fengOutput = reqbody["fengOutput"].asString();
    const string pingInput = reqbody["pingInput"].asString();
    const string pingOutput = reqbody["pingOutput"].asString();
    const string guInput = reqbody["guInput"].asString();
    const string guOutput = reqbody["guOutput"].asString();

    // 数据库操作
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    // 操作数据库
    bool existRecord{ false };
    ElectricityPriceDb << "UPDATE ELECTRICITY_PRICE_TYPELIST SET "
                            "JIAN_INPUT = ?, JIAN_OUTPUT = ?, "
                            "FENG_INPUT = ?, FENG_OUTPUT = ?, "
                            "PING_INPUT = ?, PING_OUTPUT = ?, "
                            "GU_INPUT = ?, GU_OUTPUT = ? "
                            "WHERE NAME = ?;"
                        << jianInput << jianOutput
                        << fengInput << fengOutput
                        << pingInput << pingOutput
                        << guInput << guOutput
                        << typeListName;

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("ElectricityPriceTypeListPut");
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
    if(!returnStatus) throw std::runtime_error("server operation failed");

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

std::string TypeList::createTable()
{
    string finalResult;

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
        ElectricityPriceDb << "CREATE TABLE IF NOT EXISTS ELECTRICITY_PRICE_TYPELIST("
                              "NAME TEXT PRIMARY KEY,"
                              "JIAN_INPUT TEXT,"
                              "JIAN_OUTPUT TEXT,"
                              "FENG_INPUT TEXT,"
                              "FENG_OUTPUT TEXT,"
                              "PING_INPUT TEXT,"
                              "PING_OUTPUT TEXT,"
                              "GU_INPUT TEXT,"
                              "GU_OUTPUT TEXT)";
        finalResult =  filename;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return string();
    }
    return finalResult;
}

bool TypeList::insertIntoDefaultRecord()
{
    bool finalResult;
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/ElectricityPrice.sqlite" };
        sqlite::database ElectricityPriceDb(filename);

        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_TYPELIST ("
                              "NAME, "
                              "JIAN_INPUT, JIAN_OUTPUT, "
                              "FENG_INPUT, FENG_OUTPUT, "
                              "PING_INPUT, PING_OUTPUT, "
                              "GU_INPUT, GU_OUTPUT) "
                              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);"
                            << "typeList1"
                            << "1.400" << "1.400"
                            << "1.300" << "1.300"
                            << "1.200" << "1.200"
                            << "1.100" << "1.100";
        ElectricityPriceDb << "INSERT INTO ELECTRICITY_PRICE_TYPELIST ("
                              "NAME, "
                              "JIAN_INPUT, JIAN_OUTPUT, "
                              "FENG_INPUT, FENG_OUTPUT, "
                              "PING_INPUT, PING_OUTPUT, "
                              "GU_INPUT, GU_OUTPUT) "
                              "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);"
                            << "typeList2"
                            << "1.410" << "1.410"
                            << "1.310" << "1.310"
                            << "1.210" << "1.210"
                            << "1.110" << "1.110";
        finalResult = true;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return false;
    }
    return finalResult;
}

TypeList::Record TypeList::getRecord(const string& name) const
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/ElectricityPrice.sqlite" };
    sqlite::database ElectricityPriceDb(filename);

    std::optional<Record> optValue;
    // 查询
    ElectricityPriceDb << "SELECT "
                            "JIAN_INPUT, JIAN_OUTPUT, "
                            "FENG_INPUT, FENG_OUTPUT, "
                            "PING_INPUT, PING_OUTPUT, "
                            "GU_INPUT, GU_OUTPUT "
                            "FROM ELECTRICITY_PRICE_TYPELIST "
                            "WHERE NAME = ?;"
                        << name
    >> [&](string jianInput_, string jianOutput_,
            string fengInput_, string fengOutput_,
            string pingInput_, string pingOutput_,
            string guInput_, string guOutput_){
                
                optValue.emplace(jianInput_, jianOutput_,
                                fengInput_, fengOutput_,
                                pingInput_, pingOutput_,
                                guInput_, guOutput_
                                );
            };
    if(!optValue.has_value())
        throw std::runtime_error("record not exists");
    return optValue.value();
}
