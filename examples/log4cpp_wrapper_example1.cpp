#include "cat/utils/Log4cppWrapper.h"
#include "cat/utils/YamlcppWrapper.h"
#include <algorithm>
using namespace std;
using namespace log4cpp_wrapper;

int main()
{
    // 日志参数准备
    YAML::Node logCfg;
    logCfg["category"].push_back("category1");
    logCfg["category"].push_back("category2");
    logCfg["category"].push_back("category3");
    logCfg["path"] = "./";
    logCfg["level"] = "DEBUG";

    // 日志功能初始化
    const auto& categoryCfg = logCfg["category"];
    vector<string> categorys;
    std::transform(categoryCfg.begin(), categoryCfg.end(),
        back_inserter(categorys), [](const YAML::Node& item){
        return item.as<string>();
    });
    Object::init(categorys,
                 logCfg["path"].as<string>(),
                 convertPriorityLevelFromString(logCfg["level"].as<string>()));
    
    // 用户接口使用
    auto& logger1 = Object::getLogger(0);
    logger1.debugStream() << "hello world";
    
    auto& logger2 = Object::getLogger(1);
    logger2.debugStream() << "hello world2";

    auto& logger3 = Object::getLogger(2);
    logger3.debugStream() << "hello world3";

}
