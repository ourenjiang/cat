#include "cat/utils/EnvironmentVariables.h"
#include <iostream>

int main(int argc, char** argv)
{
    environment_variables::Object::init({ "key1", "key2" });
    auto& environmentMap = environment_variables::Object::getEnvironmentMap();

    for(auto& item: environmentMap)
    {
        std::cout << static_cast<int>(item.first) << " => "
        << item.second.first << " => " << item.second.second << std::endl;
    }
}
