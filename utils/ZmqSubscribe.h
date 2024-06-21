#pragma once
#include "zmq.hpp"
#include <string>

namespace ems
{
using namespace std;

class ZmqSubscribe
{
public:
    ZmqSubscribe(const string& _addr);
    bool recv(string& ostring);
    void subscribe(const string&);
    void unsubscribe(const string&);
    zmq::socket_t& socket(){ return socket_; }
private:
    void init();
    const string addr_;
    zmq::context_t context_;
    zmq::socket_t socket_;
};

}//namespace ems
