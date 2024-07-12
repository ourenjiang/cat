#pragma once
#include <stdexcept>
#include <string>

namespace ems
{
using namespace std;

class AuthException: public std::runtime_error
{
public:
    AuthException(const std::string& msg, const std::string& username) 
        : std::runtime_error(msg), username_(username) {}
    
    // const char* what() const noexcept override {
    //     return extraInfo.c_str();
    // }
    string username() const { return username_; }
private:
     string username_;
};
}
