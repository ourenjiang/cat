#include "EnvironmentVariables.h"
#include <algorithm>
#include "boost/assert.hpp"
using namespace environment_variables;

map<EnvironmentKey, EnvironmentValue> Object::environmentMap_;

void Object::init(initializer_list<string> _list)
{
    // transform(_list.begin(), _list.end(), 
    //           inserter(environmentMap_, environmentMap_.end()),
    //           [&_list](const string& item){
    //     // auto key = static_cast<EnvironmentKey>(index);

    //     // char* envptr = std::getenv(item.c_str());
    //     // BOOST_ASSERT(envptr);
    //     // return make_pair(key, EnvironmentValue({ item, envptr}));
    //     return make_pair(1, make_pair( "", ""));
    // });
    for(auto itr = _list.begin(); itr != _list.end(); ++itr)
    {
        auto key = static_cast<EnvironmentKey>(std::distance(_list.begin(), itr));

        char* envptr = std::getenv(itr->c_str());
        BOOST_ASSERT(envptr);
        environmentMap_.emplace(key, EnvironmentValue({ *itr, envptr}));
    }
}
