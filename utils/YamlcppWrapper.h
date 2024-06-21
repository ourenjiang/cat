#pragma once
#include "yaml-cpp/yaml.h"
#include <mutex>

namespace ems
{
using namespace std;

class YamlcppWrapper
{
public:
    YamlcppWrapper() = delete;
    static void init(const string& filename);
    static YAML::Node& getRoot(){ return root_; }
private:
    inline static std::mutex mtx_;
    inline static YAML::Node root_;
};

}//namespace ems
