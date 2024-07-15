#pragma once
#include <utility>
#include "utils/HttpWrapper.h"
#include "ems/station/Model.h"
#include "zmq.hpp"

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
    static void createTable();

    using Record = std::tuple<string, string, string, string, string, string>;
    void requestCallbackGet(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
    void requestCallbackGetPageInfo(const httplib::Request &req, httplib::Response &res,
                                    shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
private:
    vector<Record> getRecord(const string pageSize, const string pageIndex,
                                const string level, const string deviceName,
                                const string beginTime, const string endTime,
                                const string action, const string processed) const;
    tuple<string, string, string> getRecordPageInfo(const string pageSize, const string level,
                                                    const string deviceName,
                                                    const string beginTime, const string endTime,
                                                    const string action, const string processed) const;
};

}//namespace ems
