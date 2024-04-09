#include "cat/utils/ZmqWrapper.h"
#include <iostream>
using namespace std;

int main()
{
    zmq_wrapper::SocketRespond server("tcp://*:6000");

    while(true)
    {
        string recvBuffer;
        bool recvResult = server.recv(recvBuffer);
        if(recvResult)
        {
            cout << "recv success"  << endl;
        }
        else
        {
            cout << "recv failed" << endl;
            continue;
        }

        string sendMsg = recvBuffer + " world";
        bool sendResult = server.send(sendMsg.data(), sendMsg.size());
        if(sendResult)
        {
            cout << "send success" << endl;
        }
        else
        {
            cout << "send failed" << endl;
            continue;
        }

    }
}