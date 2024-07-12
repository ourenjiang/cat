#include "TypeList.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "sqlite_modern_cpp.h"
#include "json/json.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/interface/OperationRecord.h"
#include "ems/base/UserManager.h"
#include "utils/AuthException.h"
#include "utils/datetime.h"
#include "utils/jsonWrapper.h"

using namespace ems;
using namespace ems::electricity_price;

void TypeList::createTable()
{
try
{
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
}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
}
}

void TypeList::insertIntoDefaultRecord()
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
        int recordCount{0};
        ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_TYPELIST WHERE NAME = ?;"
                            << "typeList1" >> recordCount;
        if(recordCount == 0){
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
        }
    }
    {
        int recordCount{0};
        ElectricityPriceDb << "SELECT COUNT(*) FROM ELECTRICITY_PRICE_TYPELIST WHERE NAME = ?;"
                            << "typeList2" >> recordCount;
        if(recordCount == 0){
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
        }
    }
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void TypeList::requestCallbackPost(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */

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

    // 这里只做通知，不同步获取结果.
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

void TypeList::requestCallbackGet(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
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

void TypeList::requestCallbackGetNameList(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
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

void TypeList::requestCallbackDelete(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */
    const string typeListName = reqbody["name"].asString();
    
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

    // 这里只做通知，不同步获取结果.
    // stationDealer->send(zmq::message_t(deleteSubtitle_), zmq::send_flags::sndmore);
    // stationDealer->send(zmq::message_t(), zmq::send_flags::none);
    
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

void TypeList::requestCallbackPut(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(reqbody);/* 鉴权 */

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

    // 这里只做通知，不同步获取结果.
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

TypeList::Record TypeList::getRecord(const string& name)
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
