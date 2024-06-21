#include "YamlcppWrapper.h"
#include <iostream>
using namespace ems;

void YamlcppWrapper::init(const string& filename)
{
    std::lock_guard<mutex> lock(mtx_);
    try{
        root_ = YAML::LoadFile(filename);
    }
    catch(YAML::BadConversion& e){
        cerr << "yamlcpp_wrapper error: " << e.what() << endl;
        throw e;// 不处理
    }
}
