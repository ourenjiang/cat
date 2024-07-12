#pragma once
#include <string>
#include <optional>

namespace cgi
{
using namespace std;

// 从CGI获取POST参数
optional<string> getRequestBody(const string& method);

}////namespace cgi
