#include "HeapSystem.h"
#include "utils/YamlcppWrapper.h"
#include "boost/assert.hpp"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"

using namespace ems;
using namespace ems::bau;

HeapSystem::HeapSystem()
    : log_(ems::Log4cppWrapper::getLogger(5))
{
    registerAllInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void HeapSystem::registerAllInterfaces()
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/heapSystem", bind(&HeapSystem::requestCallback, this, _1, _2));
}

void HeapSystem::requestCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("index"))
        throw std::runtime_error("request params err");
    
    tuple<string> workParams{ req.get_param_value("index") };

    // 准备请求参数
    auto serializedMsg = msgpackWrapper::pack(workParams);
    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("HeapSystemGet");
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

Json::Value HeapSystem::get_base(const BauStatusSummary& bauStatusSummary) const
{
    Json::Value content;
    content["bingjiNum"] = to_string(bauStatusSummary.bcuBingjiNum);
    content["soc"] = to_string(bauStatusSummary.soc);
    content["soh"] = to_string(bauStatusSummary.soh);
    content["allowChargeCapacity"] = miscellaneous::formatedPrecision(bauStatusSummary.allowChargeCapacity, 2);
    content["allowDischargeCapacity"] = miscellaneous::formatedPrecision(bauStatusSummary.allowDischargeCapacity, 2);
    content["volt"] = miscellaneous::formatedPrecision(bauStatusSummary.volt, 2);
    content["cur"] = miscellaneous::formatedPrecision(bauStatusSummary.cur, 2);
    content["clusterNum"] = to_string(bauStatusSummary.bcuNum);
    content["onlineNum"] = to_string(bauStatusSummary.bcuOnlineNum);
    content["cellNum"] = to_string(bauStatusSummary.bcuNum
                                    * bauStatusSummary.bcuSize
                                    * bauStatusSummary.bmuCellVoltSize
                                    );
    return content;
}

Json::Value HeapSystem::get_powerEnvSystem() const
{
    Json::Value content;
    content["key"] = "Reserved";
    return content;
}

Json::Value HeapSystem::get_bcu(const map<int, BcuInfo>& bcuMap) const
{
    Json::Value content;

    for(const auto& item: bcuMap){
        Json::Value branch = get_bcu(item.second);
        content.append(branch);
    }
    return content;
}

Json::Value HeapSystem::get_bcu(const BcuInfo& bcuInfo) const
{
    Json::Value content;

    const auto& bcuStatus{ bcuInfo.base };
    content["status"] = bcuStatus.gridStatus ? "Grid": "OffGrid";
    content["volt"] = miscellaneous::formatedPrecision(bcuStatus.volt, 2);
    content["cur"] = miscellaneous::formatedPrecision(bcuStatus.cur, 2);
    {
        const double power{ bcuStatus.volt * bcuStatus.cur };
        const double powerKw{ power * 0.001 };
        content["power"] = miscellaneous::formatedPrecision(powerKw, 2);
    }
    content["soc"] = to_string(bcuStatus.soc);
    content["soh"] = to_string(bcuStatus.soh);
    content["cellvoltMax"] = miscellaneous::formatedPrecision(bcuStatus.cellvoltMax, 2);
    content["cellvoltMin"] = miscellaneous::formatedPrecision(bcuStatus.cellvoltMin, 2);
    content["celltemMax"] = miscellaneous::formatedPrecision(bcuStatus.celltemMax, 1);
    content["celltemMin"] = miscellaneous::formatedPrecision(bcuStatus.celltemMin, 1);
    return content;
}

Json::Value HeapSystem::get_warning(const BauStatusSummary& bauStatusSummary) const
{
    Json::Value content;
    content["level1"] = to_string(bauStatusSummary.warnCountL1);
    content["level2"] = to_string(bauStatusSummary.warnCountL2);
    content["level3"] = to_string(bauStatusSummary.warnCountL3);
    return content;
}

string HeapSystem::convertCellAddrFormat(const uint16_t cellGlobalIndex)  const
{
    const int bcuCapacity{ 20 };
    const int bmuCapacity{ 64 };
    const int bcuIndex{ cellGlobalIndex / (bcuCapacity * bmuCapacity) };
    const int bmuIndex{ cellGlobalIndex % (bcuCapacity * bmuCapacity) / bmuCapacity };
    const int cellIndex{ cellGlobalIndex % bmuCapacity };
    return to_string(bcuIndex + 1)
           + '/' + to_string(bmuIndex + 1)
           + '/' + to_string(cellIndex + 1);
}

vector<byte> HeapSystem::respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody) const
{
    tuple<string> reqbody;
    const bool unpackResult = msgpackWrapper::unpack(msgbody.data(), msgbody.size(), reqbody);
    auto& [branchIndex] = reqbody;

    auto& branchList = stationInfo->branchList;
    auto branchItr = branchList.find(std::stoi(branchIndex));
    if(branchItr == branchList.end())
        throw std::invalid_argument("invalid params");

    const auto& branchInfo = branchItr->second;
    const auto& bauInfo = branchInfo->bauInfo;

    Json::Value root;
    root["base"] = get_base(bauInfo.bauStatusSummary);
    root["powerEnvSystem"] = get_powerEnvSystem();
    root["branch"] = get_bcu(bauInfo.bcuList);
    root["warning"] = get_warning(bauInfo.bauStatusSummary);
    root["peak"] = get_peak(bauInfo.bauStatusSummary);

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const string jsonString = Json::writeString(builder, root);
    return { reinterpret_cast<const byte*>(jsonString.data()),
                reinterpret_cast<const byte*>(jsonString.data()) + jsonString.size() };
}

Json::Value HeapSystem::get_peak(const BauStatusSummary& bauStatusSummary) const
{
    Json::Value content;

    {
        Json::Value obj;
        obj["value"] = miscellaneous::formatedPrecision(bauStatusSummary.cellvoltMax, 2);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.cellvoltMaxAddr);
        content["cellvoltMax"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = miscellaneous::formatedPrecision(bauStatusSummary.cellvoltMin, 2);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.cellvoltMinAddr);
        content["cellvoltMin"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = miscellaneous::formatedPrecision(bauStatusSummary.celltemMax, 1);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.celltemMaxAddr);
        content["celltemMax"] = obj;
    }
    {
        Json::Value obj;
        obj["value"] = miscellaneous::formatedPrecision(bauStatusSummary.celltemMin, 1);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.celltemMinAddr);
        content["celltemMin"] = obj;
    }
    return content;
}
