#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "utils/ZmqRequest.h"
#include "Model.h"

namespace ems
{
using namespace std;

enum class ActionType: uint8_t
{
    Enable, // 告警产生
    Disable,// 告警消失
    Unchange// 告警不变
};

struct BauStatusProtectStatus
{
    BauStatusProtectStatus();
    void flushBits(const uint32_t oldValue, const uint32_t newValue);
    vector<tuple<int, string, ActionType>> bits_;
};


class HistoryWarning
{
public:
    using Record = std::tuple<string, string, string, string, string, string>;
    HistoryWarning();
    vector<byte> respondCallbackGet(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    vector<byte> respondCallbackGetPageInfo(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg) const;
    vector<byte> respondCallbackGetWarningDeviceTree(std::shared_ptr<StationInfo> stationInfo, const vector<byte>& reqmsg);

    void handleBauStatusSummary(const int branchIndex, const bau::BauStatusSummary& oldValue, const bau::BauStatusSummary& newValue);
private:
    void registerHttpInterfaces(); // 数据发布
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetPageInfo(const httplib::Request &req, httplib::Response &res);
    void requestCallbackGetWarningDeviceTree(const httplib::Request &req, httplib::Response &res);

    void createTable();
    void insertRecord(const string content, const string level, const string deviceName, const string createTime, const string action, const string processed);
    vector<Record> getRecord(const string pageSize, const string pageIndex,
                                const string level, const string deviceName,
                                const string beginTime, const string endTime,
                                const string action, const string processed) const;
    tuple<string, string, string> getRecordPageInfo(const string pageSize, const string level,
                                                    const string deviceName,
                                                    const string beginTime, const string endTime,
                                                    const string action, const string processed) const;

    std::shared_ptr<ZmqRequest> requester_;
};

}//namespace ems
