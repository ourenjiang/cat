#include "zeromq.h"

zmq::context_t& zeromq::contextSingleton()
{
    static zmq::context_t context;
    return context;
}

zmq::socket_t zeromq::socket(zmq::socket_type type)
{
    auto& context = contextSingleton();
    return zmq::socket_t(context, type);
}
