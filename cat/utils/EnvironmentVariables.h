#include <map>
#include <string>
#include <initializer_list>

namespace environment_variables
{
using namespace std;

enum class EnvironmentKey
{
    Env_1,
    Env_2
};
using EnvironmentValue = pair<string, string>;
using EnvironmentMap = map<EnvironmentKey, EnvironmentValue>;

class Object
{
public:
    Object() = delete;
    static void init(initializer_list<string> _list);
    static EnvironmentMap& getEnvironmentMap(){ return environmentMap_; }
private:
    static EnvironmentMap environmentMap_;
};

}