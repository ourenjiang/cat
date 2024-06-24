#pragma once
#include <memory>
#include <vector>
#include <thread>
#include "modbus/modbus.h"
#include "utils/Log4cppWrapper.h"
#include "utils/ZmqRespond.h"
#include "utils/SyncSocketRequest.h"

namespace ems
{
namespace bau
{

class PollForward
{
public:
    PollForward();
    ~PollForward();
    void start();
private:
    optional<vector<byte>> pollModbusSlave(const zmq::message_t& msg);
    log4cpp::Category& log_;
    zmq::context_t zmqContext_;
    zmq::socket_t zmqRouter_;
    std::shared_ptr<SyncSocketRequest> syncSocket_;
    std::thread loopThread_;

};

}//namespace bau
}//namespace ems
