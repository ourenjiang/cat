#include "OperationRecord.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems;

OperationRecord::OperationRecord()
{
    registerHttpInterfaces();
    createTable();
    insertIntoDefaultRecord();

    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void OperationRecord::registerHttpInterfaces()
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/operationRecord", httplib::Server::Handler(bind(&OperationRecord::requestCallbackGet, this, _1, _2)));
    serv.Get("/operationRecordPageInfo", httplib::Server::Handler(bind(&OperationRecord::requestCallbackGetPageInfo, this, _1, _2)));
}

void OperationRecord::requestCallbackGet(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("pageSize") || !req.has_param("pageIndex"))
        throw std::invalid_argument("invalid params");
    const string pageSize = req.get_param_value("pageSize");
    const string pageIndex = req.get_param_value("pageIndex");

    const string status = req.has_param("status") ? req.get_param_value("status") : "";
    const string type = req.has_param("type") ? req.get_param_value("type") : "";
    const string beginTime = req.has_param("beginTime") ? req.get_param_value("beginTime") : "";
    const string endTime = req.has_param("endTime") ? req.get_param_value("endTime") : "";
    const string userName = req.has_param("userName") ? req.get_param_value("userName") : "";

    // 准备请求参数
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/OperationRecord.sqlite" };
    sqlite::database OperationRecordDb(filename);

    const string patternStatus = status + "%";
    const string patternType = type + "%";
    const string patternBeginTimestamp = beginTime.empty() ? "1900-01-01 00:00:00" : beginTime;
    const string patternEndTimestamp = endTime.empty() ? "2050-01-01 23:59:59" : endTime;
    
    const string patternUserName = userName + "%";

    const string patternLimit = pageSize;
    const string patternOffset = to_string(std::stoi(pageSize) * std::stoi(pageIndex));

    // 查询
    vector<OperationRecordInfo> records;
    OperationRecordDb << "SELECT "
                            "STATUS, CONTENT, TYPE, CREATE_TIME, USERNAME "
                            "FROM OPERATION_RECORD "
                            "WHERE STATUS LIKE ? AND TYPE LIKE ? "
                            "AND CREATE_TIME >= ? AND CREATE_TIME <= ? "
                            "AND USERNAME LIKE ? "
                            "LIMIT ? OFFSET ?"
                        << patternStatus << patternType
                        << patternBeginTimestamp << patternEndTimestamp
                        << patternUserName
                        << patternLimit << patternOffset
    >> [&](string selectedStatus, string selectedContent,
            string selectedType, string selectedCreateTime,
            string selectedUserName){
                
                records.emplace_back(selectedStatus, selectedContent,
                                selectedType, selectedCreateTime,
                                selectedUserName);
            };
    if(records.empty())
        throw std::runtime_error("record not exists");

    // 再解析自定义的响应内容
    // vector<OperationRecordInfo> records;
    // const bool customUnpackResult = msgpackWrapper::unpack(returnContent.data(), returnContent.size(), records);
    // BOOST_ASSERT(customUnpackResult);

    Json::Value root;
    for(const auto& item: records){
        Json::Value node;
        node["status"] = std::get<0>(item);
        node["content"] = std::get<1>(item);
        node["type"] = std::get<2>(item);
        node["createTime"] = std::get<3>(item);
        node["userName"] = std::get<4>(item);
        root.append(node);
    }

    Json::Value respondContent;
    respondContent["data"] = root;
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

vector<byte> OperationRecord::respondCallbackGet(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    tuple<string, string, string, string, string, string, string> params;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), params);

    auto [ pageSize, pageIndex, status, type, beginTime, endTime, userName ] = params;

    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/OperationRecord.sqlite" };
    sqlite::database OperationRecordDb(filename);

    const string patternStatus = status + "%";
    const string patternType = type + "%";
    const string patternBeginTimestamp = beginTime.empty() ? "1900-01-01" : beginTime;
    const string patternEndTimestamp = endTime.empty() ? "2050-01-01" : endTime;
    const string patternUserName = userName + "%";

    const string patternLimit = pageSize;
    const string patternOffset = to_string(std::stoi(pageSize) * std::stoi(pageIndex));

    // 查询
    vector<OperationRecordInfo> records;
    OperationRecordDb << "SELECT "
                            "STATUS, CONTENT, "
                            "TYPE, CREATE_TIME, "
                            "USERNAME "
                            "FROM OPERATION_RECORD "
                            "WHERE STATUS LIKE ? "
                            "AND TYPE LIKE ? "
                            "AND CREATE_TIME > ? "
                            "AND CREATE_TIME < ? "
                            "AND USERNAME LIKE ? "
                            "LIMIT ? OFFSET ?"
                        << patternStatus << patternType
                        << patternBeginTimestamp << patternEndTimestamp
                        << patternUserName
                        << patternLimit << patternOffset
    >> [&](string selectedStatus, string selectedContent,
            string selectedType, string selectedCreateTime,
            string selectedUserName){
                
                records.emplace_back(selectedStatus, selectedContent,
                                selectedType, selectedCreateTime,
                                selectedUserName);
            };
    if(records.empty())
        throw std::runtime_error("record not exists");
    auto serializedMsg = msgpackWrapper::pack(records);
    return { reinterpret_cast<const byte*>(serializedMsg.data()),
                reinterpret_cast<const byte*>(serializedMsg.data()) + serializedMsg.size() };
}

vector<byte> OperationRecord::respondCallbackGetPageInfo(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    tuple<string, string, string, string, string, string> reqbody;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), reqbody);
    auto& [ pageSize, status, type, beginTime, endTime, userName ] = reqbody;

    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/OperationRecord.sqlite" };
    sqlite::database OperationRecordDb(filename);

    vector<OperationRecordInfo> records;

    const string patternStatus = status + '%';
    const string patternType = type + '%';
    const string patternBeginTimestamp = beginTime.empty() ? "1900-01-01" : beginTime;
    const string patternEndTimestamp = endTime.empty() ? "2050-01-01" : endTime;
    const string patternUserName = userName + '%';

    // 查询
    int recordCounts;
    OperationRecordDb << "SELECT COUNT(*) "
                            "FROM OPERATION_RECORD "
                            "WHERE STATUS LIKE ? "
                            "AND TYPE LIKE ? "
                            "AND CREATE_TIME > ? "
                            "AND CREATE_TIME < ? "
                            "AND USERNAME LIKE ? "
                        << patternStatus << patternType
                        << patternBeginTimestamp << patternEndTimestamp
                        << patternUserName
    >> recordCounts;
    
    if(recordCounts > 0){

    }
    int pageNum = recordCounts / stoi(pageSize);
    if(recordCounts > 0){
        if(pageNum == 0){
            pageNum = 1;
        }
        else if(recordCounts % stoi(pageSize) > 0){
            pageNum += 1;
        }
    }

    Json::Value root;
    root["totalCount"] = to_string(recordCounts);
    root["pageNum"] = to_string(pageNum);
    root["pageSize"] = pageSize;
    return miscellaneous::serializedJsonAsBytes(root);
}

void OperationRecord::requestCallbackGetPageInfo(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("pageSize"))
        throw std::invalid_argument("invalid params");
    const string pageSize = req.get_param_value("pageSize");

    const string status = req.has_param("status") ? req.get_param_value("status") : "";
    const string type = req.has_param("type") ? req.get_param_value("type") : "";
    const string beginTime = req.has_param("beginTime") ? req.get_param_value("beginTime") : "";
    const string endTime = req.has_param("endTime") ? req.get_param_value("endTime") : "";
    const string userName = req.has_param("userName") ? req.get_param_value("userName") : "";

    // 准备请求参数
    tuple<string, string, string, string, string, string> requestMsg{ pageSize, status, type, beginTime, endTime, userName };
    auto serializedMsg = msgpackWrapper::pack(requestMsg);
    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("OperationRecordPageInfoGet");
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

    // 再解析自定义的响应内容
    const string respondContent(reinterpret_cast<const char*>(returnContent.data()),
                                reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());
    
    // 成功响应
    Json::Value repJson;
    repJson["data"] = miscellaneous::unserializedJson(respondContent);
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

std::string OperationRecord::createTable()
{
    string finalResult;

    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/OperationRecord.sqlite" };
        sqlite::database OperationRecordDb(filename);

        /**
         * 后期优化注解：
         *      1, 拼接列定义时，可以先使用vector将其缓存，然后再与SQL语句进行组合
         *      2, jian_input等列实际业务数据类型应为浮点类型，当前原型开发阶段暂时统一为字符串类型
         *      3, ...
        */
        OperationRecordDb << "CREATE TABLE IF NOT EXISTS OPERATION_RECORD("
                              "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
                              "STATUS TEXT,"
                              "CONTENT TEXT,"
                              "TYPE TEXT,"
                              "CREATE_TIME TEXT,"
                              "USERNAME TEXT)";
        finalResult =  filename;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return string();
    }
    return finalResult;
}

void OperationRecord::insertIntoDefaultRecord()
{
    try
    {
        /* code */
        const string projectPath{ "/opt/paceic_ems_server/main" };
        const string dbPath{ projectPath + "/db" };
        BOOST_ASSERT(filesystem::is_directory(dbPath));

        const string filename{ dbPath + "/OperationRecord.sqlite" };
        sqlite::database OperationRecordDb(filename);

        OperationRecordDb << "DELETE FROM OPERATION_RECORD;";// clear old records

        OperationRecordDb << "INSERT INTO OPERATION_RECORD (STATUS, CONTENT, TYPE, CREATE_TIME, USERNAME) VALUES (?, ?, ?, ?, ?);"
                            << "成功" << "测试内容1" << "参数设置" << "2024-06-12" << "root";
        OperationRecordDb << "INSERT INTO OPERATION_RECORD (STATUS, CONTENT, TYPE, CREATE_TIME, USERNAME) VALUES (?, ?, ?, ?, ?);"
                            << "成功" << "测试内容2" << "设备控制" << "2024-06-13" << "admin";
        OperationRecordDb << "INSERT INTO OPERATION_RECORD (STATUS, CONTENT, TYPE, CREATE_TIME, USERNAME) VALUES (?, ?, ?, ?, ?);"
                            << "失败" << "测试内容3" << "用户管理" << "2024-06-14" << "test";
    }
    catch(const std::exception& e){
        std::cerr << e.what() << '\n';
    }
}

void OperationRecord::insertRecord(const string& status, const string& content, const string& type, const string& username)
{
try
{
    const string nowTimestamp = miscellaneous::getCurrentTimestamp();
    
    const string projectPath{ "/opt/paceic_ems_server/main" };
    const string dbPath{ projectPath + "/db" };
    BOOST_ASSERT(filesystem::is_directory(dbPath));

    const string filename{ dbPath + "/OperationRecord.sqlite" };
    sqlite::database OperationRecordDb(filename);

    OperationRecordDb << "INSERT INTO OPERATION_RECORD ("
                            "STATUS, "
                            "CONTENT, TYPE, "
                            "CREATE_TIME, USERNAME) "
                            "VALUES (?, ?, ?, ?, ?);"
                        << status
                        << content << type
                        << nowTimestamp << username;
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}
