#include "utils/SyncSocketRequest.h"
#include <thread>
#include <iostream>
#include <vector>

using namespace std;
using namespace ems;

int main(int argc, char **argv)
{
    SyncSocketRequest syncSocket("192.168.49.1", "9001");


    while(true)
    {
        // 01 03 04 06 00 5B E5 00

        vector<uint8_t> rawFrame{ 0x01, 0x03, 0x04, 0x06, 0x00, 0x5B, 0xE5, 0x00 };
        const string frameString(rawFrame.begin(), rawFrame.end());
        syncSocket.asyncWrite(frameString);
        const bool recvResult = syncSocket.syncReadConditionVariable();
        if(recvResult){

            const string recvMessage = syncSocket.gerRecvBuffer();
            cout << "recv " << recvMessage.size() << " bytes" << endl;
            this_thread::sleep_for(chrono::milliseconds(500));
        }
    }
}
