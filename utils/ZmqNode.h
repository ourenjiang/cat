/*
Zmq节点，包含一个Dealer和一个Router套接字,
允许该节点对外发起连接，同时接收外部连接。
*/
#pragma once
#include "utils/Miscellaneous.h"

namespace ems
{
using namespace std;

class ZmqNode
{
public:
    ZmqNode(const string& port);
    std::shared_ptr<zmq::socket_t> router(){ return router_; }
    std::shared_ptr<zmq::socket_t> createDealer(const string& identity);
    void recv(zmq::message_t& frame);
    void send(zmq::message_t& frame, zmq::send_flags flag);
    void send(zmq::message_t&& frame, zmq::send_flags flag);
    bool getSockoptRcvmore();
private:
    std::shared_ptr<zmq::socket_t> router_;
    const string address_;
};
};//namespace ems