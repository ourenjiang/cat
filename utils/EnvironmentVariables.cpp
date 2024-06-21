#include "EnvironmentVariables.h"
#include <algorithm>
#include "boost/assert.hpp"

using namespace ems;

void EnvironmentVariables::init(initializer_list<string> _list)
{
    for(auto itr = _list.begin(); itr != _list.end(); ++itr)
    {
        auto key = static_cast<EnvironmentKey>(std::distance(_list.begin(), itr));

        char* envptr = std::getenv(itr->c_str());
        BOOST_ASSERT(envptr);
        environmentMap_.emplace(key, EnvironmentValue({ *itr, envptr}));
    }
}
