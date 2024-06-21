#pragma once
#include "utils/Log4cppWrapper.h"
#include "utils/HttpWrapper.h"

namespace ems
{
using namespace std;

class UserManager
{
public:
    UserManager();
    static bool doAuth(const string& username, const string& password);
private:
    bool insertDefault();
    void registerCallbacks();
    
    using Request = httplib::Request;
    using Response = httplib::Response;
    void getCallback(const Request&, Response&);
    void getNameListCallback(const Request&, Response&);
    void getAllCallback(const Request&, Response&);
    void postCallback(const Request&, Response&);
    void putCallback(const Request&, Response&);
    void deleteCallback(const Request&, Response&);

    log4cpp::Category& log_;
};
}//namespace ems
