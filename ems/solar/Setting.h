#pragma once
#include "utils/HttpWrapper.h"

namespace ems
{
namespace solar
{
using namespace std;

class Setting
{
public:
    Setting();
    bool setStatus(const string& name, const string& status);
    bool setPowerLimit(const string& name,const string& powerLimit);

    // 数据发布
    void registerHttpInterfaces();
    void methodGetSetStatusCallback(const httplib::Request &req, httplib::Response &res);
    void methodGetSetPowerLimitCallback(const httplib::Request &req, httplib::Response &res);
};

}//namespace solar
}//namespace ems
