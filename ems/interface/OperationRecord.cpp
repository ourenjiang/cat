#include "OperationRecord.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"
#include "sqlite_modern_cpp.h"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"
#include "utils/jsonWrapper.h"

using namespace ems;

void OperationRecord::requestCallbackGet(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    if(!reqbody.isMember("pageSize") || !reqbody.isMember("pageIndex"))
        throw std::invalid_argument("invalid params");
    const string pageSize = reqbody["pageSize"].asString();
    const string pageIndex = reqbody["pageIndex"].asString();

    const string status = reqbody.isMember("status") ? reqbody["status"].asString() : "";
    const string type = reqbody.isMember("type") ? reqbody["type"].asString() : "";
    const string beginTime = reqbody.isMember("beginTime") ? reqbody["beginTime"].asString() : "";
    const string endTime = reqbody.isMember("endTime") ? reqbody["endTime"].asString() : "";
    const string userName = reqbody.isMember("userName") ? reqbody["userName"].asString() : "";

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

void OperationRecord::requestCallbackGetPageInfo(const httplib::Request &req, httplib::Response &res, shared_ptr<zmq::socket_t> stationDealer)
{
try
{
    const auto reqbody = json_wrapper::deserialize(req.body);
    if(!reqbody.isMember("pageSize"))
        throw std::invalid_argument("invalid params");
    const string pageSize = reqbody["pageSize"].asString();

    const string status = reqbody.isMember("status") ? reqbody["status"].asString() : "";
    const string type = reqbody.isMember("type") ? reqbody["type"].asString() : "";
    const string beginTime = reqbody.isMember("beginTime") ? reqbody["beginTime"].asString() : "";
    const string endTime = reqbody.isMember("endTime") ? reqbody["endTime"].asString() : "";
    const string userName = reqbody.isMember("userName") ? reqbody["userName"].asString() : "";

    // 数据库操作
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

void OperationRecord::createTable()
{
try
{
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
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
}
}

void OperationRecord::insertIntoDefaultRecord()
{
try
{
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
