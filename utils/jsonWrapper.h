#pragma once
#include <sstream>
#include <vector>
#include "json/json.h"

namespace json_wrapper
{
using namespace std;

Json::Value deserialize(const string& str);// 反序列化
string serialize(const Json::Value& root);// 序列化

}////namespace miscellaneous
