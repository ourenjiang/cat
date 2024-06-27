#pragma once
#include <tuple>
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "ems/bau/BmuPoller.h"
#include "utils/ZmqRequest.h"
#include "ems/station/Model.h"

namespace ems
{
namespace bau
{
using namespace std;

class CellSystem
{
public:
    CellSystem();
    void respondCallback(std::shared_ptr<StationInfo> stationInfo, zmq::socket_t& router,
                        const vector<byte>& identity, const vector<byte>& subtitle, const vector<byte>& body) const;
    vector<byte> identity();
private:
    void registerAllInterfaces();
    void requestCallback(const httplib::Request &req, httplib::Response &res);

    static Json::Value get_statistic(const CellvoltSummary&, const CelltemSummary&);//统计数据
    static Json::Value get_cellvolt(const CellvoltSummary&);//单体电压
    static Json::Value get_celltem(const CelltemSummary&);//单体温度
    static Json::Value get_terminaltem(const CelltemSummary&);//端子温度

    const string identity_;
    log4cpp::Category& log_;
    zmq::socket_t dealer_;
};
}//namespace bau
}//namespace ems
