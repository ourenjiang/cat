#pragma once
#include "utils/HttpWrapper.h"
#include "ems/bau/Model.h"
#include "ems/station/Model.h"
#include "zmq.hpp"

namespace ems
{
namespace bau
{
using namespace std;

class HeapSystem
{
public:
    static void requestCallback(const httplib::Request &req, httplib::Response &res,
                        shared_ptr<zmq::socket_t> dataDealer, shared_ptr<zmq::socket_t> cmdDealer);
private:
    static string convertCellAddrFormat(const uint16_t);
    static Json::Value get_base(const BauStatusSummary&);//基础信息
    static Json::Value get_powerEnvSystem();//动环信息
    static Json::Value get_bcu(const map<int, BcuInfo>&);
    static Json::Value get_bcu(const BcuInfo&);
    static Json::Value get_warning(const BauStatusSummary&);//告警信息
    static Json::Value get_peak(const BauStatusSummary&);//系统极值
};
}//namespace bau
}//namespace ems
