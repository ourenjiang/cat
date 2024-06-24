#include "StorageEnergySystem.h"
#include "utils/YamlcppWrapper.h"
#include "boost/assert.hpp"
#include "utils/MsgpackWrapper_src.hpp"
#include "utils/Miscellaneous.h"

using namespace ems;

StorageEnergySystem::StorageEnergySystem()
    : log_(ems::Log4cppWrapper::getLogger(5))
{
    registerAllInterfaces();
    requester_ = make_unique<ZmqRequest>("tcp://127.0.0.1:6200");
}

void StorageEnergySystem::registerAllInterfaces()
{
    using std::placeholders::_1;
    using std::placeholders::_2;

    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/storageEnergySystem", bind(&StorageEnergySystem::requestCallback, this, _1, _2));
}

void StorageEnergySystem::requestCallback(const httplib::Request &req, httplib::Response &res)
{
try
{
    // Json::Value respondmsg;
    // respondmsg["errcode"] = 0;
    // respondmsg["errmsg"] = "success";

    // auto handleResult = handleGetHttp();
    // if(handleResult.first){

    //     string& jsonString = handleResult.second;

    //     JSONCPP_STRING err;
    //     Json::Value root;
    //     Json::CharReaderBuilder builder;
    //     const unique_ptr<Json::CharReader> reader(builder.newCharReader());
    //     if (!reader->parse(jsonString.data(), jsonString.data() + jsonString.length(), &root, &err)){
    //         throw std::invalid_argument("parse json string err");
    //     }
        
    //     respondmsg["data"] = root;
    // }
    // utils::httpRespond(res, respondmsg);

    // 准备请求消息
    string publishContent;
    const string topic = miscellaneous::createFixedSizeString("StorageEnergySystemGet");
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
    Json::Value respondmsg;
    respondmsg["errcode"] = -1;
    respondmsg["errmsg"] = e.what();
    utils::httpRespond(res, respondmsg);
}
}

pair<bool, string> StorageEnergySystem::handleGetHttp()
{
    try
    {
        const string topic = miscellaneous::createFixedSizeString("StorageEnergySystemGet");

        string requestMessage;
        std::copy(topic.data(), topic.data() + topic.size(), std::back_inserter(requestMessage));

        const vector<byte> sendmsg(reinterpret_cast<byte*>(requestMessage.data()),
                                reinterpret_cast<byte*>(requestMessage.data()) + requestMessage.size());
        const bool sendResult = requester_->send(sendmsg);
        if(!sendResult){
            throw std::runtime_error("send err");
        }

        // 接收
        const auto recvResult = requester_->recv();
        if(!recvResult.has_value()){
            throw std::runtime_error("recv err");
        }
        const auto recvmsg = recvResult.value();

        pair<bool, string> respondMsg;
        const bool unpackResult = msgpackWrapper::unpack(recvmsg.data(), recvmsg.size(), respondMsg);
        return respondMsg;
    }
    catch(const std::exception& e){
        return { false, e.what() };
    }
    return {};
}

Json::Value StorageEnergySystem::get_trend(const StationInfo& stationInfo)
{
    Json::Value content;
    content["soc"] = get_trend_soc(stationInfo);
    content["cur"] = get_trend_cur(stationInfo);
    content["power"] = get_trend_power(stationInfo);
    return content;
}

Json::Value StorageEnergySystem::get_trend_soc(const StationInfo& stationInfo)
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

Json::Value StorageEnergySystem::get_trend_cur(const StationInfo& stationInfo)
{
    Json::Value content;
    // auto& stroageInfo = storage_->getStationInfo();
    auto& snapshot = stationInfo.curRealtimeSnapshot;
    for(const auto& item: snapshot){
        Json::Value obj;
        obj["time"] = item.first;
        obj["value"] = item.second;
        content.append(obj);
    }
    return content;
}

Json::Value StorageEnergySystem::get_trend_power(const StationInfo& stationInfo)
{
    Json::Value content;
    // auto& stroageInfo = storage_->getStationInfo();
    auto& snapshot = stationInfo.powerRealtimeSnapshot;
    for(const auto& item: snapshot){
        Json::Value obj;
        obj["time"] = item.first;
        obj["value"] = item.second;
        content.append(obj);
    }
    return content;
}

Json::Value StorageEnergySystem::get_centralTopArea(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    content["totalVolt"] = miscellaneous::formatedPrecision(bauStatusSummary.volt, 2);
    content["totalCur"] = miscellaneous::formatedPrecision(bauStatusSummary.cur, 2);
    {
        const double totalPower = (bauStatusSummary.volt) * (bauStatusSummary.cur);
        const double totalPowerKw = totalPower * 0.001;
        content["totalPower"] = miscellaneous::formatedPrecision(totalPowerKw, 2);
    }
    content["allowChargeCapacity"] = miscellaneous::formatedPrecision(bauStatusSummary.allowChargeCapacity, 2);
    content["allowDischargeCapacity"] = miscellaneous::formatedPrecision(bauStatusSummary.allowDischargeCapacity, 2);
    content["heapNum"] = "1";
    return content;
}

Json::Value StorageEnergySystem::get_branch(const StationInfo& stationInfo)
{
    Json::Value content;
    const int branchNum = 1;//暂时写死

    for(int index = 0; index < branchNum; ++index){
        Json::Value branch;

        auto itr = stationInfo.branchList.find(index);
        if(itr != stationInfo.branchList.end()){

            const auto& branchInfo = itr->second;
            const auto& pcsInfo = branchInfo->pcsInfo;
            branch["pcs"] = get_branch_pcs(pcsInfo.frame_0406_0460_summary);

            const auto& bauInfo = branchInfo->bauInfo;
            branch["bau"] = get_branch_bau(bauInfo.bauStatusSummary);
            content.append(branch);
        }
    }
    return content;
}

Json::Value StorageEnergySystem::get_branch_pcs(const pcs::_0406_0460_Summary& summary)
{
    Json::Value content;
    
    content["status"] = "Reversed";

    {
        const string gridLineVoltAB = miscellaneous::formatedPrecision(summary.gridLineVoltAB, 1);
        const string gridLineVoltBC = miscellaneous::formatedPrecision(summary.gridLineVoltBC, 1);
        const string gridLineVoltCA = miscellaneous::formatedPrecision(summary.gridLineVoltCA, 1);
        content["volt"] = gridLineVoltAB + '/' + gridLineVoltBC + '/' + gridLineVoltCA;
    }
    {
        const string gridCurA = miscellaneous::formatedPrecision(summary.gridCurA, 1);
        const string gridCurB = miscellaneous::formatedPrecision(summary.gridCurB, 1);
        const string gridCurC = miscellaneous::formatedPrecision(summary.gridCurC, 1);
        content["cur"] = gridCurA + '/' + gridCurB + '/' + gridCurC;
    }
    {
        const string gridTotalActivePower = miscellaneous::formatedPrecision(summary.gridTotalActivePower, 1);
        const string gridTotalReactivePower = miscellaneous::formatedPrecision(summary.gridTotalReactivePower, 1);
        const string gridTotalApparentPower = miscellaneous::formatedPrecision(summary.gridTotalApparentPower, 1);
        content["power"] = gridTotalActivePower + '/' + gridTotalReactivePower + '/' + gridTotalApparentPower;
    }
    return content;
}

Json::Value StorageEnergySystem::get_branch_bau(const bau::BauStatusSummary& bauStatusSummary)
{
    Json::Value content;
    content["soc"] = miscellaneous::formatedPrecision(bauStatusSummary.soc, 0);
    content["soh"] = miscellaneous::formatedPrecision(bauStatusSummary.soh, 0);
    content["volt"] = miscellaneous::formatedPrecision(bauStatusSummary.volt, 2);
    content["cur"] = miscellaneous::formatedPrecision(bauStatusSummary.cur, 2);
    {
        const double power = (bauStatusSummary.volt) * (bauStatusSummary.cur);
        const double powerKw = power * 0.001;
        content["power"] = miscellaneous::formatedPrecision(powerKw, 2);
    }
    content["cellvoltMax"] = miscellaneous::formatedPrecision(bauStatusSummary.cellvoltMax, 2);
    content["cellvoltMin"] = miscellaneous::formatedPrecision(bauStatusSummary.cellvoltMin, 2);
    content["celltemMax"] = miscellaneous::formatedPrecision(bauStatusSummary.celltemMax, 2);
    content["celltemMin"] = miscellaneous::formatedPrecision(bauStatusSummary.celltemMin, 2);
    return content;
}

Json::Value StorageEnergySystem::get_warning(const bau::BauStatusSummary& bauStatusSummary, const pcs::_0406_0460_Summary& pcs_0406_0460_summary)
{
    Json::Value content;
    content["currentNum"] = getWarningNumOfBau(bauStatusSummary) + getWarningNumOfPcs(pcs_0406_0460_summary);
    content["todayAddNum"] = "Reserved";

    content["level1"] = to_string(bauStatusSummary.warnCountL1);
    content["level2"] = to_string(bauStatusSummary.warnCountL2);
    content["level3"] = to_string(bauStatusSummary.warnCountL3);
    return content;
}

size_t StorageEnergySystem::getWarningNumOfBau(const bau::BauStatusSummary& bauStatusSummary)
{
    return bauStatusSummary.warnCountTotal;
}

size_t StorageEnergySystem::getWarningNumOfPcs(const pcs::_0406_0460_Summary& summary)
{
    return summary.warnCountTotal;
}

string StorageEnergySystem::convertCellAddrFormat(const uint16_t cellGlobalIndex)
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

Json::Value StorageEnergySystem::get_peak(const bau::BauStatusSummary& bauStatusSummary)
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

vector<byte> StorageEnergySystem::respondCallback(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg)
{
    Json::Value root;
    root["trend"] = StorageEnergySystem::get_trend(*stationInfo);

    const auto& branchInfo = stationInfo->branchList[0];// 暂时只统计第一分支
    const auto& bauStatusSummary = branchInfo->bauInfo.bauStatusSummary;
    root["centralTopArea"] = StorageEnergySystem::get_centralTopArea(bauStatusSummary);
    root["branch"] = StorageEnergySystem::get_branch(*stationInfo);

    const auto& pcs_frame_0406_0460_summary = branchInfo->pcsInfo.frame_0406_0460_summary;
    root["warning"] = StorageEnergySystem::get_warning(bauStatusSummary, pcs_frame_0406_0460_summary);
    root["peak"] = StorageEnergySystem::get_peak(bauStatusSummary);
    return miscellaneous::serializedJsonAsBytes(root);
}
