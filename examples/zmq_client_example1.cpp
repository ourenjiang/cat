#include "cat/utils/ZmqWrapper.h"
#include <iostream>
using namespace std;

int main()
{
    zmq_wrapper::SocketRequest client("tcp://localhost:6000");

    while(true)
    {

        string sendMsg("hello");
        bool sendResult = client.send(sendMsg.data(), sendMsg.size());
        if(sendResult)
        {
            cout << "send success" << endl;
        }
        else
        {
            cout << "send failed" << endl;
            continue;
        }

        string recvBuffer;
        bool recvResult = client.recv(recvBuffer);
        if(recvResult)
        {
            cout << "recv success"  << endl;
        }
        else
        {
            cout << "recv failed" << endl;
        }
    }
}
