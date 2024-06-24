#include "Setting.h"
#include <filesystem>
#include "boost/assert.hpp"
#include "json/json.h"

using namespace ems;
using namespace ems::solar;

Setting::Setting()
{
    registerHttpInterfaces();
}

bool Setting::setStatus(const string& name, const string& status)
{
    return true;
}

bool Setting::setPowerLimit(const string& name, const string& powerLimit)
{
    return true;
}

void Setting::registerHttpInterfaces()
{
    using namespace std::placeholders;
    auto& serv = utils::getHttpServerSingleton();
    serv.Get("/solar/setStatus", httplib::Server::Handler(bind(&Setting::methodGetSetStatusCallback, this, _1, _2)));
    serv.Get("/solar/setPowerLimit", httplib::Server::Handler(bind(&Setting::methodGetSetPowerLimitCallback, this, _1, _2)));
}

void Setting::methodGetSetStatusCallback(const httplib::Request &req, httplib::Response &res)
{
    Json::Value respondContent;
    if(!req.has_param("name")
       || !req.has_param("status")){
        respondContent["message"] = "无效参数";
        respondContent["errcode"] = 101;
        utils::httpRespond(res, respondContent);
        return;
    }
    
    string name = req.get_param_value("name");
    string status = req.get_param_value("status");
    const bool result = setStatus(name, status);
    if(result){
        respondContent["errcode"] = 0;
        respondContent["errmsg"] = "success";
    }
    else{
        respondContent["errcode"] = -1;
        respondContent["errmsg"] = "failed";
    }
    utils::httpRespond(res, respondContent);
}

void Setting::methodGetSetPowerLimitCallback(const httplib::Request &req, httplib::Response &res)
{
    Json::Value respondContent;

    if(!req.has_param("name")
       || !req.has_param("powerLimit")){
        respondContent["message"] = "无效参数";
        respondContent["errcode"] = 101;
        utils::httpRespond(res, respondContent);
        return;
    }
    
    string name = req.get_param_value("name");
    string powerLimit = req.get_param_value("powerLimit");
    const bool result = setPowerLimit(name, powerLimit);
    if(result){
        respondContent["errcode"] = 0;
        respondContent["errmsg"] = "success";
    }
    else{
        respondContent["errcode"] = -1;
        respondContent["errmsg"] = "failed";
    }
    utils::httpRespond(res, respondContent);
}
