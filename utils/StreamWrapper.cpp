#include "StreamWrapper.h"
#include <regex>
#include <iomanip>// setprecision

using namespace std;

bool stream_wrapper::matchTimeHHMM(const string& timestr)
{
    regex timeRegex("^([01]?[0-9]|2[0-3]):([0-5][0-9])$");
    return regex_match(timestr, timeRegex);
}

vector<byte> stream_wrapper::toBytes(const string& data)
{
    return { reinterpret_cast<const byte*>(data.data()),
            reinterpret_cast<const byte*>(data.data()) + data.size() };
}

vector<byte> stream_wrapper::toBytes(const void* data, size_t len)
{
    const auto byteptr = static_cast<const byte*>(data);
    return { byteptr, byteptr + len };
}

string stream_wrapper::toString(const void* data, size_t len)
{
    return { static_cast<const char*>(data), len };
}

string stream_wrapper::serializeFloat(const double rawData, int formatedPrecision)
{
    ostringstream oss;
    oss << std::fixed << std::setprecision(formatedPrecision) << rawData;
    return oss.str();
}

string stream_wrapper::fixedSize(const string& content, const size_t len)
{
    string newObj(content);
    newObj.resize(len);
    return newObj;
}



