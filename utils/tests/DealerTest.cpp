#include "zmq.hpp"
#include <thread>
#include <iostream>

using namespace std;
using namespace std::chrono;

int main()
{

    //发送
    thread th1_([&]{

    const string addr("tcp://127.0.0.1:5557");
    zmq::context_t context_;
    zmq::socket_t sock_(context_, zmq::socket_type::dealer);

    sock_.connect(addr);
    while(true){

        string content("hello world");
        zmq::message_t msg(content);
        auto sendResult = sock_.send(msg, zmq::send_flags::none);
        // auto sendResult = sock_.send(msg);
        if(sendResult){
            // cout << "send success" << endl;

            zmq::message_t msg;
            auto recvResult = sock_.recv(msg);
            if(recvResult.has_value()){
                cout << "recv size: " << recvResult.value() << endl;
                cout << "recv data: " << msg.to_string() << endl;
                assert(msg.to_string() == content);
            }
        }
        this_thread::sleep_for(milliseconds(10));
    }
    });
    /////////////////////////////////////////////////////////////

    //发送
    thread th12_([&]{

    const string addr("tcp://127.0.0.1:5557");
    zmq::context_t context_;
    zmq::socket_t sock_(context_, zmq::socket_type::dealer);

    sock_.connect(addr);
    while(true){

        string content("github.com");
        zmq::message_t msg(content);
        auto sendResult = sock_.send(msg, zmq::send_flags::none);
        // auto sendResult = sock_.send(msg);
        if(sendResult){
            // cout << "send success" << endl;

            zmq::message_t msg;
            auto recvResult = sock_.recv(msg);
            if(recvResult.has_value()){
                // cout << "recv size: " << recvResult.value() << endl;
                // cout << "recv data: " << msg.to_string() << endl;
            }
        }
        this_thread::sleep_for(milliseconds(10));
    }
    });
    ///////////////////////////////////////////////////////////

    // 接收
    thread th2_([&]{

    const string addr("tcp://127.0.0.1:5557");
    zmq::context_t context_;
    zmq::socket_t sock_(context_, zmq::socket_type::router);

    sock_.set(zmq::sockopt::rcvtimeo, 1000);

    sock_.bind(addr);
    while(true){

        zmq::message_t msg;
        auto recvResult = sock_.recv(msg);
        if(recvResult.has_value()){
            // cout << "recv size: " << recvResult.value() << endl;
            // cout << "recv data: " << msg.to_string() << endl;

            auto sendResult = sock_.send(msg, zmq::send_flags::none);
            if(sendResult){
                // cout << "send success" << endl;
            }
        }
        else{
            cout << "not recved" << endl;
        }
    }
    });

    th1_.join();
    th12_.join();
    th2_.join();
}
