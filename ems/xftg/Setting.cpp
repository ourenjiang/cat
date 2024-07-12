#include "Setting.h"
#include "boost/assert.hpp"
#include <filesystem>
#include "sqlite_modern_cpp.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "ems/base/UserManager.h"
#include "ems/interface/OperationRecord.h"
#include "utils/AuthException.h"
#include "utils/jsonWrapper.h"
#include "utils/zeromq.h"
#include "ems/base/StationInfo.h"

using namespace ems;
using namespace ems::xftg;

void Setting::requestCallbackGet(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    if(!req.has_param("branchIndex"))
        throw std::runtime_error("invalud params");
    const string branchIndex = req.get_param_value("branchIndex");

    auto stationInfo = base::getStationInfo(stationDealer);
    auto& strategyInfo = stationInfo.strategyMap_.at(std::stoi(branchIndex));

    Json::Value data;
    data["autoRun"] = strategyInfo.autoRun ? "true" : "false";
    data["activeStrategyName"] = strategyInfo.activeStrategy;

    Json::Value respondContent;
    respondContent["data"] = data;
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

void Setting::requestCallbackPut(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    const auto body = json_wrapper::deserialize(req.body);
    const string usernameAuth = base::UserManager::doAuth(body);/* 鉴权 */
    const string branchIndex = base::UserManager::getParam(body, "branchIndex");
    const string autoRunFlag = base::UserManager::getParam(body, "autoRun");

    // 操作数据库
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    // 检查记录是否存在
    int recordCount{ 0 };
    XftgDb << "SELECT COUNT(*) FROM AUTO_RUN WHERE BRANCH_INDEX = ?;"
        << branchIndex >> recordCount;
    if(recordCount == 0)    
        throw std::runtime_error("request params err");
    
    // 执行修改
    XftgDb << "UPDATE AUTO_RUN SET FLAG = ? WHERE BRANCH_INDEX = ?;"
            << autoRunFlag << branchIndex;

    // 通知中心
    stationDealer->send(zmq::message_t(), zmq::send_flags::sndmore);
    stationDealer->send(zmq::message_t(string("Strategy0")), zmq::send_flags::sndmore);
    stationDealer->send(zmq::message_t(string("Interface")), zmq::send_flags::sndmore);
    stationDealer->send(zmq::message_t(string("AutoRun")), zmq::send_flags::none);
    // 响应
    zmq::message_t deviceBody;
    (void)stationDealer->recv(deviceBody);

    // 解析消息
    pair<bool, vector<uint8_t>> respondMsg;
    const bool unpackMsgResult = msgpackWrapper::unpack(deviceBody.data(), deviceBody.size(), respondMsg);
    BOOST_ASSERT(unpackMsgResult);
    const auto& [returnStatus, returnContent] = respondMsg;
    if(!returnStatus)
        throw std::runtime_error("modbus respond failed");

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

void Setting::createTable()
{
try
{
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/Xftg.sqlite" };
    sqlite::database XftgDb(filename);

    XftgDb << "CREATE TABLE IF NOT EXISTS AUTO_RUN("
                "BRANCH_INDEX TEXT PRIMARY KEY,"
                "FLAG TEXT);";
}
catch(const sqlite::sqlite_exception& e){
    std::cerr << e.get_sql() << '\n';
}
}

void Setting::insertIntoDefaultRecord()
{
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
