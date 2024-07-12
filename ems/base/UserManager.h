#pragma once
#include <string>
#include "json/json.h"

namespace ems
{
namespace base
{
using namespace std;

class UserManager
{
public:
    UserManager();
    static string doAuth(const Json::Value& body);
    static bool doAuth(const string& username, const string& password);
    static string getParam(const Json::Value& root, const string& key);
};
}//namespace base
}//namespace ems
