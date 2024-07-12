#include "ZmqNode.h"
#include "zeromq.h"

using namespace std;
using namespace ems;

ZmqNode::ZmqNode(const string& port)
    : router_(make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::router))
    , address_("tcp://127.0.0.1:" + port)
{
    router_->bind(address_);
}

shared_ptr<zmq::socket_t> ZmqNode::createDealer(const string& identity)
{
    auto dealer = make_shared<zmq::socket_t>(zeromq::contextSingleton(), zmq::socket_type::dealer);
    dealer->set(zmq::sockopt::routing_id, identity);
    dealer->connect(address_);
    return dealer;
}

void ZmqNode::recv(zmq::message_t& frame)
{
    (void)router_->recv(frame);
}

void ZmqNode::send(zmq::message_t& frame, zmq::send_flags flag)
{
    router_->send(frame, flag);
}

void ZmqNode::send(zmq::message_t&& frame, zmq::send_flags flag)
{
    router_->send(std::move(frame), flag);

}

bool ZmqNode::getSockoptRcvmore()
{
    return router_->get(zmq::sockopt::rcvmore);
}
