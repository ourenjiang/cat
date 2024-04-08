#pragma once
#include "yaml-cpp/yaml.h"

namespace yamlcpp_wrapper
{
using namespace std;

class Object
{
public:
    Object() = delete;
    static void init(const string& filename);
    static YAML::Node& getRoot(){ return root_; }
private:
    static YAML::Node root_;
};

}//namespace yamlcpp_wrapper
