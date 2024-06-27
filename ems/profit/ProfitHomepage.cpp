#include "ProfitHomepage.h"
#include "utils/YamlcppWrapper.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"

using namespace ems;

Profit::Profit()
    : log_(ems::Log4cppWrapper::getLogger(5))
    , identity_("ProfitHomepageGet")
    , dealer_(miscellaneous::createZmqSocket(zmq::socket_type::dealer))
{
    registerAllInterfaces();
    dealer_.set(zmq::sockopt::routing_id, identity_);
    dealer_.connect("tcp://127.0.0.1:6200");
}

vector<byte> Profit::identity()
{
    const auto beginItr = reinterpret_cast<const byte*>(identity_.data());
    return { beginItr, beginItr + identity_.size() };
}

void Profit::registerAllInterfaces()
{
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/profit", bind(&Profit::requestCallback, this, _1, _2));
}

void Profit::requestCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    dealer_.send(zmq::message_t(), zmq::send_flags::sndmore);
    dealer_.send(zmq::message_t(), zmq::send_flags::none);
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

void Profit::respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body)
{
try
{
    Json::Value root;

    /**
     * 枚举值【策略模式】：
     * 101 - 两充两放
     * 102 - 三充三放
    */
    root["strategyMode"] = "Reserved";

    /**
     * 能量W(kWH) = 电量Q(AH) * 电压V(V) * 0.001
    */
    // const auto& StationInfo = storage_->getStationInfo();
    // const auto& bauInfo = StationInfo->branchList[0]->bauInfo;

    // const double chargeSum = bauInfo->bauStatusSummary->chargeCapacitySum;
    // const double dischargeSum = bauInfo->bauStatusSummary->dischargeCapacitySum;
    // const double volt = bauInfo->bauStatusSummary->volt;

    // const double chargeSumKwh = chargeSum * volt * 0.001;
    // const double dischargeSumKwh = dischargeSum * volt * 0.001;

    root["profitToday"] = "Reserved";//今日收益
    root["profitTotal"] = "Reserved";//累计收益

    const auto& branchInfo = stationInfo->branchList[0];// 暂时只统计第一分支
    const auto& bauStatusSummary = branchInfo->bauInfo.bauStatusSummary;

    root["soc"] = to_string(bauStatusSummary.soc);
    root["chargeCapacityToday"] = "Reserved";     //今日充电量
    root["dischargeCapacityToday"] = "Reserved";  //今日放电量
    root["chargeCapacityTotal"] = miscellaneous::formatedPrecision(bauStatusSummary.chargeCapacitySum, 2);//累计充电量
    root["dischargeCapacityTotal"] = miscellaneous::formatedPrecision(bauStatusSummary.dischargeCapacitySum, 2);//累计放电量
    // merge_warningCountsByGrade(data);
    // merge_bauWarningCounts(data);
    // merge_pcsWarningCounts(data);
    root["bmsMode"] = "Reserved";
    root["allowChargeCapacity"] = miscellaneous::formatedPrecision(bauStatusSummary.allowChargeCapacity, 2);
    root["allowDischargeCapacity"] = miscellaneous::formatedPrecision(bauStatusSummary.allowDischargeCapacity, 2);
    root["deviceOnlineCounts"] = "Reserved";

    auto& snapshot = stationInfo->powerRealtimeSnapshot;
    for(const auto& item: snapshot){
        Json::Value obj;
        obj["time"] = item.first;
        obj["value"] = item.second;
        root["realPower"].append(obj);
    }

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
