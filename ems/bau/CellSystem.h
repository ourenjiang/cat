#pragma once
#include <tuple>
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"
#include "zmq.hpp"
#include "ems/station/Model.h"

namespace ems
{
namespace bau
{
using namespace std;

class CellSystem
{
public:
    void requestCallback(const httplib::Request &req, httplib::Response &res,
                        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
private:
    static Json::Value get_statistic(const CellvoltSummary&, const CelltemSummary&);//统计数据
    static Json::Value get_cellvolt(const CellvoltSummary&);//单体电压
    static Json::Value get_celltem(const CelltemSummary&);//单体温度
    static Json::Value get_terminaltem(const CelltemSummary&);//端子温度
};
}//namespace bau
}//namespace ems
