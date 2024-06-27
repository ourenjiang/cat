#include "HeapSystem.h"
#include "utils/YamlcppWrapper.h"
#include "boost/assert.hpp"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"

using namespace ems;
using namespace ems::bau;

HeapSystem::HeapSystem()
    : log_(ems::Log4cppWrapper::getLogger(5))
    , identity_("HeapSystemGet")
    , dealer_(miscellaneous::createZmqSocket(zmq::socket_type::dealer))
{
    registerAllInterfaces();
    dealer_.set(zmq::sockopt::routing_id, identity_);
    dealer_.connect("tcp://127.0.0.1:6200");
}

vector<byte> HeapSystem::identity()
{
    const auto beginItr = reinterpret_cast<const byte*>(identity_.data());
    return { beginItr, beginItr + identity_.size() };
}

void HeapSystem::registerAllInterfaces()
{
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/heapSystem", bind(&HeapSystem::requestCallback, this, _1, _2));
}

void HeapSystem::requestCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    if(!req.has_param("branchIndex"))
        throw std::runtime_error("request params err");
    
    // 准备请求参数
    const string workParams = req.get_param_value("branchIndex");
    auto serializedMsg = msgpackWrapper::pack(workParams);
    zmq::message_t sndmsg(serializedMsg.data(), serializedMsg.size());
    dealer_.send(zmq::message_t(), zmq::send_flags::sndmore);
    dealer_.send(sndmsg, zmq::send_flags::none);
    // 接收消息
    zmq::message_t rcvmsg;
    (void)dealer_.recv(rcvmsg);

    // 先解析标准的响应消息
    pair<bool, vector<byte>> respondMsg;
    const bool unpackMsgResult = msgpackWrapper::unpack(rcvmsg.data(), rcvmsg.size(), respondMsg);
    BOOST_ASSERT(unpackMsgResult);
    const auto& [returnStatus, returnContent] = respondMsg;
    if(!returnStatus){
        // 再解析自定义的响应内容
        const string errmsg(reinterpret_cast<const char*>(returnContent.data()),
                                    reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());
        throw std::runtime_error(errmsg);
    }

    // 再解析自定义的响应内容
    const string jsonString(reinterpret_cast<const char*>(returnContent.data()),
                                reinterpret_cast<const char*>(returnContent.data()) + returnContent.size());

    // 成功响应
    Json::Value repJson;
    repJson["data"] = miscellaneous::unserializedJson(jsonString);
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
    Json::Value content(Json::arrayValue);

    for(const auto& item: bcuMap){
        Json::Value info = get_bcu(item.second);
        info["index"] = to_string(item.first);
        content.append(info);
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

void HeapSystem::respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body) const
{
try
{
    string reqbody;
    const bool unpackResult = msgpackWrapper::unpack(body.data(), body.size(), reqbody);
    const string branchIndex = reqbody;

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

    {
        const vector<byte> repcontent = miscellaneous::serializedJsonAsBytes(root);
        const pair<bool, vector<byte>> repbody{ true, repcontent };
        const msgpack::sbuffer repbodySerialized = msgpackWrapper::pack(repbody);

        zmq::message_t identitymsg(identity.data(), identity.size());
        zmq::message_t repmsg(repbodySerialized.data(), repbodySerialized.size());
        router.send(identitymsg, zmq::send_flags::sndmore);
        router.send(repmsg, zmq::send_flags::none);
    }
}
catch(const std::exception& e){
    const vector<byte> repcontent = miscellaneous::convertStringToBytes(e.what());
    const pair<bool, vector<byte>> repbody{ false, repcontent };
    const msgpack::sbuffer repbodySerialized = msgpackWrapper::pack(repbody);

    zmq::message_t identitymsg(identity.data(), identity.size());
    zmq::message_t repmsg(repbodySerialized.data(), repbodySerialized.size());
    router.send(identitymsg, zmq::send_flags::sndmore);
    router.send(repmsg, zmq::send_flags::none);
}
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
