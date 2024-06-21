#pragma once
#include <map>
#include <string>
#include <initializer_list>

namespace ems
{
using namespace std;

enum class EnvironmentKey
{
    Env_1,
    Env_2
};
using EnvironmentValue = pair<string, string>;
using EnvironmentMap = map<EnvironmentKey, EnvironmentValue>;

class EnvironmentVariables
{
public:
    EnvironmentVariables() = delete;
    static void init(initializer_list<string> _list);
    static EnvironmentMap& getEnvironmentMap(){ return environmentMap_; }
private:
    inline static EnvironmentMap environmentMap_;
};

}//namespace ems