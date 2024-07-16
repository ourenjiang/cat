#include "zmq.hpp"
#include <thread>
#include <iostream>

using namespace std;
using namespace std::chrono;

#include "utils/HttpWrapper.h"
#include <iostream>

/*
 1，N 对 1
 2，没有类似请求/响应的强制同步要求；
 3，跟踪消息所属套接字的状态；

*/

// int main()
// {
//     httplib::Client cli("http://192.168.126.1:8000");

//     auto res = cli.Get("/httplib.h");
//     if (res && res->status == 200) {
//         std::cout << res->body << std::endl;
//     }
// }

int main()
{

    //发送
    thread th1_([&]{

    const string addr("tcp://127.0.0.1:5657");
    zmq::context_t context_;
    zmq::socket_t sock_(context_, zmq::socket_type::dealer);

    sock_.connect(addr);
    while(true){

        {
            zmq::message_t delimiter;
            auto sendDelimiterResult = sock_.send(delimiter, zmq::send_flags::sndmore);
            assert(sendDelimiterResult);

            string content("hello world");
            zmq::message_t msg(content);
            auto sendContentResult = sock_.send(msg, zmq::send_flags::none);
            assert(sendContentResult);
        }

        {
            // zmq::message_t delimiter;
            // auto recvDelimiterResult = sock_.recv(delimiter);
            // assert(recvDelimiterResult);

            zmq::message_t content;
            auto recvContentResult = sock_.recv(content);
            assert(recvContentResult);

            const string contentString(static_cast<char*>(content.data()), content.size());
            cout << "recv: " << contentString << endl;
        }
        this_thread::sleep_for(milliseconds(10));
    }
    });
    /////////////////////////////////////////////////////////////

    //发送
    thread th2_([&]{

    const string addr("tcp://127.0.0.1:5657");
    zmq::context_t context_;
    zmq::socket_t sock_(context_, zmq::socket_type::dealer);

    sock_.connect(addr);
    while(true){

        {
            zmq::message_t delimiter;
            auto sendDelimiterResult = sock_.send(delimiter, zmq::send_flags::sndmore);
            assert(sendDelimiterResult);

            string content("hello world2");
            zmq::message_t msg(content);
            auto sendContentResult = sock_.send(msg, zmq::send_flags::none);
            assert(sendContentResult);
        }

        {
            // zmq::message_t delimiter;
            // auto recvDelimiterResult = sock_.recv(delimiter);
            // assert(recvDelimiterResult);

            zmq::message_t content;
            auto recvContentResult = sock_.recv(content);
            assert(recvContentResult);

            const string contentString(static_cast<char*>(content.data()), content.size());
            cout << "recv2: " << contentString << endl;
        }
        this_thread::sleep_for(milliseconds(10));
    }
    });
    /////////////////////////////////////////////////////////////

    // 接收
    thread th3_([&]{

    const string addr("tcp://127.0.0.1:5657");
    zmq::context_t context_;
    zmq::socket_t sock_(context_, zmq::socket_type::router);

    // sock_.set(zmq::sockopt::rcvtimeo, 1000);

    sock_.bind(addr);
    while(true){

        zmq::message_t identity;
        (void)sock_.recv(identity);
        assert(sock_.get(zmq::sockopt::rcvmore));

        zmq::message_t delimiter;
        (void)sock_.recv(delimiter);
        assert(sock_.get(zmq::sockopt::rcvmore));
        
        zmq::message_t content;
        (void)sock_.recv(content);
        assert(!sock_.get(zmq::sockopt::rcvmore));
        ////////////////////////////////////////////////////

        zmq::message_t respondContent(content.data(), content.size());
        sock_.send(identity, zmq::send_flags::sndmore);
        sock_.send(respondContent, zmq::send_flags::none);
    }
    });

    th1_.join();
    th2_.join();
    th3_.join();
}
