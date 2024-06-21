#include <iostream>
#include <thread>
#include "utils/SyncSocketRequest.h"

using namespace std;

int main()
{
    // std::cout << "hello world" << std::endl;

    ems::SyncSocketRequest syncSocket("192.168.10.251", "19000");
    while(true){
        syncSocket.asyncConnect();
        this_thread::sleep_for(chrono::milliseconds(10));
    }
}