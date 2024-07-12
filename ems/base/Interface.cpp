#include "Interface.h"
#include "utils/MsgpackWrapper_src.hpp"

using namespace std;

vector<byte> interface::respondMessage(const bool status, const vector<byte>& content)
{
    const pair<bool, vector<byte>> msg{status, content};
    const auto serializedData = msgpackWrapper::pack(msg);

    const byte* bytePtr = reinterpret_cast<const byte*>(serializedData.data());
    const vector<byte> bytes(bytePtr, bytePtr + serializedData.size());
    return bytes;
}
