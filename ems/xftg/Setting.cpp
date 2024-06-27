#include "Setting.h"
#include "boost/assert.hpp"
#include <filesystem>
#include "sqlite_modern_cpp.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/station/UserManager.h"
#include "utils/AuthException.h"
#include "ems/station/OperationRecord.h"

using namespace ems;
using namespace ems::xftg;

Setting::Setting()
    : identity_("XftgAutoRun")
    , dealer_(miscellaneous::createZmqSocket(zmq::socket_type::dealer))
{
    createTable();
    insertIntoDefaultRecord();
    registerHttpInterfaces();
    dealer_.set(zmq::sockopt::routing_id, identity_);
    dealer_.connect("tcp://127.0.0.1:6200");
}

vector<byte> Setting::identity()
{
    const auto beginItr = reinterpret_cast<const byte*>(identity_.data());
    return { beginItr, beginItr + identity_.size() };
}

void Setting::registerHttpInterfaces()
{
    using Handler = httplib::Server::Handler;
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/xftg/autoRun", Handler(bind(&Setting::requestCallbackGet, this, _1, _2)));
    serv.Put("/xftg/autoRun", Handler(bind(&Setting::requestCallbackPut, this, _1, _2)));
}

void Setting::requestCallbackGet(const httplib::Request &req, httplib::Response &res)
{
try
{
    const string branchIndex = req.get_param_value("branchIndex");

    const auto result = getRecord(branchIndex);
    if(!result.has_value())
        throw std::runtime_error("record not exists");

    Json::Value respondContent;
    respondContent["data"] = result.value();
    respondContent["errcode"] = 0;
    respondContent["errmsg"] = "success";
    utils::httpRespond(res, respondContent);
}
catch(const std::exception& e){
    Json::Value respondmsg;
    respondmsg["errcode"] = -2;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

void Setting::requestCallbackPut(const httplib::Request &req, httplib::Response &res)
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
    if(!reqbody.isMember("branchIndex") || !reqbody.isMember("autoRun"))
        throw std::runtime_error("request params err");
    const string branchIndex = reqbody["branchIndex"].asString();
    const string autoRunFlag = reqbody["autoRun"].asString();
    if(autoRunFlag != "true" && autoRunFlag != "false")
        throw std::runtime_error("request params err");

    // 操作数据库
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    XftgDb << "UPDATE AUTO_RUN SET FLAG = ? WHERE BRANCH_INDEX = ?;"
            << autoRunFlag << branchIndex;

    // 通知中心
    dealer_.send(zmq::message_t(), zmq::send_flags::sndmore);
    dealer_.send(zmq::message_t(), zmq::send_flags::none);

    // 成功响应
    Json::Value repJson;
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e){
    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

vector<byte> Setting::respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body) const
{
    // 这里需要从数据库重新加载这部分记录.

    return miscellaneous::convertStringToBytes("success");
}

std::string Setting::createTable()
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Xftg.sqlite" };
        sqlite::database XftgDb(filename);

        XftgDb << "CREATE TABLE IF NOT EXISTS AUTO_RUN("
                    "BRANCH_INDEX TEXT PRIMARY KEY,"
                    "FLAG TEXT);";
        return filename;
    }
    catch(const sqlite::sqlite_exception& e){
        std::cerr << e.get_sql() << '\n';
    }
    return {};
}

void Setting::insertIntoDefaultRecord()
{
    bool finalResult;
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/Xftg.sqlite" };
        sqlite::database XftgDb(filename);

        XftgDb << "DELETE FROM AUTO_RUN;";// clear old records

        XftgDb << "INSERT INTO AUTO_RUN (BRANCH_INDEX, FLAG) VALUES (?, ?);"
                << "0" << "true";
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
}

optional<string> Setting::getRecord(const string& branchIndex)
{
try
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    string flag;
    XftgDb << "SELECT FLAG FROM AUTO_RUN WHERE BRANCH_INDEX = ?;"
            << branchIndex
            >> flag;
    return flag;
}
catch(const sqlite::sqlite_exception& e){
    std::cerr << e.what() << '\n';
}
return {};
}
