#include "MainWiringDiagram.h"
#include "boost/assert.hpp"
#include "utils/YamlcppWrapper.h"
#include "utils/Miscellaneous.h"
#include "utils/MsgpackWrapper_src.hpp"

using namespace ems;

MainWiringDiagram::MainWiringDiagram()
    : log_(ems::Log4cppWrapper::getLogger(5))
{
    registerAllInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void MainWiringDiagram::registerAllInterfaces()
{
    using std::placeholders::_1;
    using std::placeholders::_2;
    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/mainWiringDiagram", bind(&MainWiringDiagram::requestCallback, this, _1, _2));
}

void MainWiringDiagram::requestCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("MainWiringDiagramGet");
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

Json::Value MainWiringDiagram::get_realPower(const StationInfo& stationInfo)
{
    Json::Value content;
    auto& snapshot = stationInfo.socRealtimeSnapshot;
    for(const auto& item: snapshot){
        Json::Value obj;
        obj["time"] = item.first;
        obj["value"] = item.second;
        content.append(obj);
    }
    return content;
}

Json::Value MainWiringDiagram::get_capacitySum(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    content["bms"] = get_capacitySum_bms(bauStatusSummary);
    content["pv"] = get_capacitySum_pv();
    return content;
}

Json::Value MainWiringDiagram::get_capacitySum_bms(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    content["charge"] = miscellaneous::formatedPrecision(bauStatusSummary.chargeCapacitySum, 1);
    content["discharge"] = miscellaneous::formatedPrecision(bauStatusSummary.dischargeCapacitySum, 1);
    return content;
}

Json::Value MainWiringDiagram::get_capacitySum_pv()
{
    Json::Value content;
    content["charge"] = "Reversed";
    content["discharge"] = "Reversed";
    return content;
}

Json::Value MainWiringDiagram::get_env()
{
    Json::Value content;
    {
        content["air"] = "Reversed";
    }
    {
        content["tem"] = "Reversed";
    }
    {
        content["fire"] = "Reversed";
    }
    {
        content["hum"] = "Reversed";
    }
    return content;
}

/**
 * 运行模式：
 *      如果安装了多个分支的情况，只要有一个分支并网成功，则显示并网，
 *      如果所有分支都处于离网状态，则显示为离网
 * 策略模式：
 *      如果只装配一个储能分支，则按实际执行情况显示；
 *      如果装配了多个分支，则显示为“混合”
*/
Json::Value MainWiringDiagram::get_mode()
{
    Json::Value content;
    content["run"] = "Reserved";
    content["strategy"] = get_mode_strategy();
    return content;
}

string MainWiringDiagram::get_mode_strategy()
{
    return "Reserved";
}

Json::Value MainWiringDiagram::get_meter()
{
    Json::Value content;
    {
        content["volt"] = "Reserved";
    }
    {
        content["cur"] = "Reserved";
    }
    {
        content["power"] = "Reserved";
    }
    return content;
}

Json::Value MainWiringDiagram::get_storageSystem(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;

    content["soc"] = to_string(bauStatusSummary.soc);
    content["status"] = "Reserved";
    content["volt"] = miscellaneous::formatedPrecision(bauStatusSummary.volt, 1);
    content["cur"] = miscellaneous::formatedPrecision(bauStatusSummary.cur, 2);
    {
        const double power{ bauStatusSummary.volt * bauStatusSummary.cur };
        const double powerKw{ power * 0.001 };
        content["power"] = miscellaneous::formatedPrecision(powerKw, 2);
    }
    return content;
}

Json::Value MainWiringDiagram::get_pvSystem()
{
    Json::Value content;
    content["status"] = "Reserved";
    content["volt"] = "Reserved";
    content["cur"] = "Reserved";
    content["power"] = "Reserved";
    content["soc"] = "Reserved";
    return content;
}

Json::Value MainWiringDiagram::get_loadSystem()
{
    Json::Value content;
    content["status"] = "Reserved";
    content["volt"] = "Reserved";
    content["cur"] = "Reserved";
    content["power"] = "Reserved";
    return content;
}

Json::Value MainWiringDiagram::get_envManage()
{
    Json::Value content;
    content["air"] = "Reserved";
    content["tem"] = "Reserved";
    content["fire"] = "Reserved";
    content["hum"] = "Reserved";
    return content;
}

Json::Value MainWiringDiagram::get_bms(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;

    content["runStatus"] = "Reserved";
    content["gridStatus"] = "Reserved";
    content["soc"] = to_string(bauStatusSummary.soc);
    content["volt"] = miscellaneous::formatedPrecision(bauStatusSummary.volt, 2);
    content["cur"] = miscellaneous::formatedPrecision(bauStatusSummary.cur, 2);
    content["dischargeCapacity"] = miscellaneous::formatedPrecision(bauStatusSummary.dischargeCapacitySum, 2);
    return content;
}

Json::Value MainWiringDiagram::get_warning(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    content["byDevice"] = get_warning_byDevice(bauStatusSummary);
    content["byGrade"] = get_warning_byGrade(bauStatusSummary);
    return content;
}

Json::Value MainWiringDiagram::get_warning_byDevice(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;

    content["bms"] = to_string(bauStatusSummary.warnCountL1 + bauStatusSummary.warnCountL2 + bauStatusSummary.warnCountL3);
    content["pcs"] = "0";
    return content;
}

Json::Value MainWiringDiagram::get_warning_byGrade(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    content["level1"] = to_string(bauStatusSummary.warnCountL1);
    content["level2"] = to_string(bauStatusSummary.warnCountL2);
    content["level3"] = to_string(bauStatusSummary.warnCountL3);
    return content;
}

string MainWiringDiagram::convertCellAddrFormat(const uint16_t cellGlobalIndex)
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

Json::Value MainWiringDiagram::get_peak(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    {
        auto& obj = content["cellvoltMax"];
        obj["value"] = miscellaneous::formatedPrecision(bauStatusSummary.cellvoltMax, 2);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.cellvoltMaxAddr);
    }
    {
        auto& obj = content["cellvoltMin"];
        obj["value"] = miscellaneous::formatedPrecision(bauStatusSummary.cellvoltMin, 2);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.cellvoltMinAddr);
    }
    {
        auto& obj = content["celltemMax"];
        obj["value"] = miscellaneous::formatedPrecision(bauStatusSummary.celltemMax, 1);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.celltemMaxAddr);
    }
    {
        auto& obj = content["celltemMin"];
        obj["value"] = miscellaneous::formatedPrecision(bauStatusSummary.celltemMin, 1);
        obj["addr"] = convertCellAddrFormat(bauStatusSummary.celltemMinAddr);
    }
    return content;
}

vector<byte> MainWiringDiagram::respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& msgbody)
{
    Json::Value root;
    root["realPower"] = MainWiringDiagram::get_realPower(*stationInfo);

    const auto& branchInfo = stationInfo->branchList[0];// 暂时只统计第一分支
    const auto& bauStatusSummary = branchInfo->bauInfo.bauStatusSummary;
    root["capacitySum"] = MainWiringDiagram::get_capacitySum(bauStatusSummary);
    root["env"] = MainWiringDiagram::get_env();
    root["mode"] = MainWiringDiagram::get_mode();
    root["meter"] = MainWiringDiagram::get_meter();
    root["storageSystem"] = MainWiringDiagram::get_storageSystem(bauStatusSummary);
    root["pvSystem"] = MainWiringDiagram::get_pvSystem();
    root["pvSystem"] = MainWiringDiagram::get_loadSystem();
    root["envManage"] = MainWiringDiagram::get_envManage();
    root["bms"] = MainWiringDiagram::get_bms(bauStatusSummary);
    root["warning"] = MainWiringDiagram::get_warning(bauStatusSummary);
    root["peak"] = MainWiringDiagram::get_peak(bauStatusSummary);
    return miscellaneous::serializedJsonAsBytes(root);
}
