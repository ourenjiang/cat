#include <iostream>
#include <thread>
#include "utils/SyncRequest.h"

using namespace std;

int main()
{
    // std::cout << "hello world" << std::endl;

    ems::SyncRequest syncSocket("192.168.49.1:9000");
    syncSocket.start();
    while(true){
        const string msg("hello world");
        const vector<byte> data(reinterpret_cast<const byte*>(msg.data()), 
            reinterpret_cast<const byte*>(msg.data()) + msg.size());
        syncSocket.syncWrite(data, 1000);
        this_thread::sleep_for(chrono::milliseconds(5));
    }
}