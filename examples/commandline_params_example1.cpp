#include "cat/utils/CommandlineParams.h"
#include <iostream>

int main(int argc, char** argv)
{
    commandline_params::Object::init(argc, argv);
    // std::cout << commandline_params::Object::getHelpInfo() << std::endl;

    auto& variablesMap = commandline_params::Object::getVariablesMap();
    if(variablesMap.find("configure") != variablesMap.end())
    {
        std::cout << variablesMap["configure"].as<std::string>() << std::endl;
    }
}
