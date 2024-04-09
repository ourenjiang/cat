#include "zmq.hpp"
#include <iostream>
using namespace std;

int main()
{
    zmq::context_t context(1);
    zmq::socket_t* client = new zmq::socket_t(context, ZMQ_REQ);
    client->connect("tcp://localhost:5558");
    int linger = 0;
    client->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));

    while(true)
    {
        string sendContent("hello");
        zmq::message_t sendmsg(sendContent.data(), sendContent.size());
        zmq::send_result_t sendResult = client->send(sendmsg, zmq::send_flags::none);
        if(sendResult.has_value())
        {
            cout << "send success" << endl;
        }
        else
        {
            cout << "send failed" << endl;
        }

        // 开始接收
        zmq::pollitem_t items[] = {
            { *client, 0, ZMQ_POLLIN, 0 }
        };
        zmq::poll (&items[0], 1, 1000);
        if(items[0].revents & ZMQ_POLLIN)
        {
            zmq::message_t message;
            zmq::recv_result_t result = client->recv(message, zmq::recv_flags::none);
            if(result.has_value())
            {
                cout << "recv: " << message.to_string() << endl;
                continue;
            }
        }
        
        cout << "接收失败" << endl;
        delete client;
        client = new zmq::socket_t(context, ZMQ_REQ);
        client->connect("tcp://localhost:5558");
        int linger = 0;
        client->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    }

}