#pragma once
#include "cpphttplib_splitout/httplib.h"
#include "json/json.h"

namespace ems
{
namespace utils
{
using namespace std;
using namespace httplib;

httplib::Server& getHttpServerSingleton();
void httpRespond(Response &res, const Json::Value data);
}
}
