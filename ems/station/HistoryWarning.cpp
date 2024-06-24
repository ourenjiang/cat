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

    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}
void HistoryWarning::registerHttpInterfaces()
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/warning", httplib::Server::Handler(bind(&HistoryWarning::requestCallbackGet, this, _1, _2)));
    serv.Get("/warningPageInfo", httplib::Server::Handler(bind(&HistoryWarning::requestCallbackGetPageInfo, this, _1, _2)));
    serv.Get("/warningDeviceTree", httplib::Server::Handler(bind(&HistoryWarning::requestCallbackGetWarningDeviceTree, this, _1, _2)));
}

void HistoryWarning::requestCallbackGet(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("pageSize") || !req.has_param("pageIndex"))
        throw std::invalid_argument("invalid params");
    const string pageSize = req.get_param_value("pageSize");
    const string pageIndex = req.get_param_value("pageIndex");

    const string level = req.has_param("level") ? req.get_param_value("level") : "";
    const string deviceName = req.has_param("deviceName") ? req.get_param_value("deviceName") : "";
    const string beginTime = req.has_param("beginTime") ? req.get_param_value("beginTime") : "";
    const string endTime = req.has_param("endTime") ? req.get_param_value("endTime") : "";
    const string actionType = req.has_param("actionType") ? req.get_param_value("actionType") : "";
    const string processed = req.has_param("processed") ? req.get_param_value("processed") : "";

    // 准备请求参数
    tuple<string, string, string,
            string, string, string, string, string> workParams{ pageSize, pageIndex,
                        level, deviceName, beginTime, endTime, actionType, processed };
    auto serializedMsg = msgpackWrapper::pack(workParams);
    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("WarningRecordGet");
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
    msg["data"] = Json::Value(Json::arrayValue);
    msg["errcode"] = -1;
    msg["errmsg"] = e.what();
    utils::httpRespond(res, msg);
}
}

vector<byte> HistoryWarning::respondCallbackGet(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    tuple<string, string, string,
        string, string, string, string, string> reqbody;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), reqbody);
    const auto& [ pageSize, pageIndex, level,
            deviceName, beginTime, endTime, actionType, processed ] = reqbody;

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
    return miscellaneous::serializedJsonAsBytes(root);
}

vector<byte> HistoryWarning::respondCallbackGetPageInfo(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    tuple<string, string, string, string, string, string, string> params;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), params);
    auto [ pageSize, level, deviceName, beginTime, endTime, actionType, processed ] = params;

    tuple<bool, vector<string>> respondResult{ false, {} };
    const auto getResult = getRecordPageInfo(pageSize, level, deviceName, beginTime, endTime, actionType, processed);

    Json::Value root;
    root["totalCount"] = std::get<0>(getResult);
    root["pageNum"] = std::get<1>(getResult);
    root["pageSize"] = std::get<2>(getResult);
    return miscellaneous::serializedJsonAsBytes(root);
}

vector<byte> HistoryWarning::respondCallbackGetWarningDeviceTree(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody)
{
    Json::Value root(Json::arrayValue);
    for(const auto& item: stationInfo->branchList){

        Json::Value branchData;
        branchData["branchIndex"] = item.first;
        {
            auto& branchInfo = item.second;
            const auto& bauInfo = branchInfo->bauInfo;
            Json::Value bauData;
            bauData["bauActive"] = bauInfo.existActiveWarningOrFault ? "true": "false";

            for(const auto& bcu: bauInfo.bcuList){

                const auto& bcuInfo = bcu.second;
                Json::Value bcuData;
                bcuData["bcuIndex"] = to_string(bcu.first);
                bcuData["bcuActive"] = bcuInfo.existActiveWarningOrFault ? "true": "false";
                bauData["bcu"].append(bcuData);
            }
            branchData["bau"] = bauData;
        }
        {
            auto& branchInfo = item.second;
            const auto& pcsInfo = branchInfo->pcsInfo;
            branchData["pcsActive"] = pcsInfo.existActiveWarningOrFault ? "true": "false";
        }
        root.append(branchData);
    }
    return miscellaneous::serializedJsonAsBytes(root);
}

void HistoryWarning::requestCallbackGetPageInfo(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("pageSize")){
        throw std::invalid_argument("invalid params");
    }
    const string pageSize = req.get_param_value("pageSize");

    const string level = req.has_param("level") ? req.get_param_value("level") : "";
    const string deviceName = req.has_param("deviceName") ? req.get_param_value("deviceName") : "";
    const string beginTime = req.has_param("beginTime") ? req.get_param_value("beginTime") : "";
    const string endTime = req.has_param("endTime") ? req.get_param_value("endTime") : "";
    const string actionType = req.has_param("actionType") ? req.get_param_value("actionType") : "";
    const string processed = req.has_param("processed") ? req.get_param_value("processed") : "";


    tuple<string, string, string, string, string, string, string> requestMsg{ pageSize,
                                            level, deviceName, beginTime, endTime, actionType, processed };
    auto serializedMsg = msgpackWrapper::pack(requestMsg);

    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("WarningRecordPageInfoGet");
    std::copy(topic.data(), topic.data() + topic.size(), std::back_inserter(publishContent));
    std::copy(serializedMsg.data(), serializedMsg.data() + serializedMsg.size(),
              std::back_inserter(publishContent));

    const vector<byte> sendmsg(reinterpret_cast<byte*>(publishContent.data()),
                                reinterpret_cast<byte*>(publishContent.data()) + publishContent.size());
    const bool sendResult = requester_->send(sendmsg);
    if(!sendResult) throw std::runtime_error("zmq send err");

    // 接收
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

void HistoryWarning::requestCallbackGetWarningDeviceTree(const httplib::Request &req, httplib::Response &res)
{
try
{
    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("WarningDeviceTree");
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
        const string errmsg(reinterpret_cast<const char*>(returnContent.data()),
                                    reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());
        throw std::runtime_error(errmsg);
    }

    // 再解析自定义的响应内容
    const string jsonString(reinterpret_cast<const char*>(returnContent.data()),
                                reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());

    Json::Value repJson;
    repJson["data"] = miscellaneous::unserializedJson(jsonString);
    repJson["errcode"] = 0;
    repJson["errmsg"] = "success";
    utils::httpRespond(res, repJson);
}
catch(const std::exception& e){
    // 失败响应
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
