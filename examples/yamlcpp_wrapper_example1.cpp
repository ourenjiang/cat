#include "cat/utils/YamlcppWrapper.h"
#include <iostream>

int main()
{
    yamlcpp_wrapper::Object::init("./config.yaml");

    auto& root = yamlcpp_wrapper::Object::getRoot();
    std::cout << root["hello"].as<std::string>() << std::endl;
}
