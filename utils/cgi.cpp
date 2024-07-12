#include "cgi.h"

using namespace std;

// 获取POST请求体
optional<string> cgi::getRequestBody(const string& method)
{
    const char* methodPtr = ::getenv("REQUEST_METHOD");
    if (!methodPtr) return {};
    const string methodStr(methodPtr);
    if (methodStr.find(method) == string::npos) return {};
    const char* contentLengthPtr = ::getenv("CONTENT_LENGTH");
    if (!contentLengthPtr) return {};
    const size_t contentLength = static_cast<size_t>(::atoi(contentLengthPtr));

    string buffer('\0', contentLength);
    ::fread(buffer.data(), contentLength, 1, stdin);
    return buffer;
}
