#include "utils/SyncRespond.h"
#include <iostream>

using namespace std;
using namespace ems;

vector<uint8_t> Callback(const vector<uint8_t>& req)
{
    cout << "recv " << req.size() << " bytes" << endl;
    return req;
};

int main()
{
    SyncRespond respond("8004", Callback);
    getchar();
}
