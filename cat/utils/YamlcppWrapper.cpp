#include "YamlcppWrapper.h"
#include <iostream>
#include "RedisWrapper.h"
using namespace yamlcpp_wrapper;

YAML::Node Object::root_;

void Object::init(const string& filename)
{
    try{
        root_ = YAML::LoadFile(filename);
    }
    catch(YAML::BadConversion& e){
        cerr << "yamlcpp_wrapper error: " << e.what() << endl;
        throw e;// 不处理
    }
}
