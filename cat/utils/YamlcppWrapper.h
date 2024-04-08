#pragma once
#include "yaml-cpp/yaml.h"

namespace yamlcpp_wrapper
{
using namespace std;

class Object
{
public:
    // 加载配置文件
    static void init(const string& filename);
    // 获取根对象
    static YAML::Node& getRoot(){ return root_; }
private:
    static YAML::Node root_;
};

}//yamlcpp_wrapper
