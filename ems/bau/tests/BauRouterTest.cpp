#include "ems/bau/BauRouter.h"
#include "utils/zeromq.h"
#include <iostream>

using namespace std;
using namespace ems;
using namespace ems::bau;

int main()
{
    // auto modbusProxy_ = make_shared<SyncSocketRequest>("192.168.49.1", "9000");
    // auto zmqRouter_ = make_shared<zmq::socket_t>(miscellaneous::getZmqContextSingleton(), zmq::socket_type::router);
    // zmqRouter_->bind("tcp://127.0.0.1:12000");
    // BauRouter router(zmqRouter_, modbusProxy_);
    BauRouter router = createPollRouter("tcp://127.0.0.1:12000", "192.168.49.1:9000");

    // vector<BauRouter> vecs;
    // vecs.push_back(router);
    router.start();

    zmq::socket_t zmqDealer_(zeromq::socket(zmq::socket_type::dealer));
    const string proxyAddress("tcp://127.0.0.1:12000");
    zmqDealer_.connect(proxyAddress);
    while(true){
        const string reqmsg("hello world");
        zmq::message_t sndmsg(reqmsg.data(), reqmsg.size());
        zmqDealer_.send(sndmsg, zmq::send_flags::none);

        zmq::pollitem_t item{ zmqDealer_, 0, ZMQ_POLLIN, 0 };
        const int pollResult = zmq::poll(&item, 1, std::chrono::seconds(1));
        if(pollResult == 0) continue;

        zmq::message_t rcvmsg;
        (void)zmqDealer_.recv(rcvmsg);
        const string rcvmsgstr(reinterpret_cast<char*>(rcvmsg.data()),
                reinterpret_cast<char*>(rcvmsg.data()) + rcvmsg.size());
        cout << "rcv: " << rcvmsgstr << endl;
    }
}
