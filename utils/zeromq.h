#pragma once
#include "zmq.hpp"

namespace zeromq
{

zmq::context_t& contextSingleton();
zmq::socket_t socket(zmq::socket_type type);

}////namespace miscellaneous
