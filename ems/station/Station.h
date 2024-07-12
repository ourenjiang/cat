#pragma once
#include "Model.h"
#include "utils/zeromq.h"
// #include "ems/branch/EnergyBranch.h"

/**
 * 两件事：
*     1，作为poller消息的订阅端；
 *    2，当策略所依赖的数据完整更新后，主动调用策略模块并获取姿态调整参数，并下发;
 *    3, 启动请求/响应连接的服务端，等待来自外部HTTP请求的转发请求，并返回给HTTP服务器;
*/
#include <thread>
#include "zmq.hpp"
#include "Model.h"

namespace ems
{
using namespace std;

class Station
{
public:
    Station();
    ~Station();
    void start();
private:
    void doWork();
    void doPublishInfo();
    void doReadInfo(zmq::message_t& identity);

    StationInfo stationInfo_;
    shared_ptr<zmq::socket_t> zmqRouter_;
    std::thread loopThread_;
};
}//namespace ems
