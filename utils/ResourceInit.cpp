#include "ResourceInit.h"
#include "utils/CommandlineParams.h"
#include "utils/YamlcppWrapper.h"
#include "utils/Log4cppWrapper.h"
#include "boost/assert.hpp"
#include <iostream>

using namespace ems::utils;

void ems::utils::commandlineParamsInit(int argc, char **argv)
{
    CommandlineParams::init(argc, argv);
    auto& variablesMap = CommandlineParams::getVariablesMap();
    if(variablesMap.find("help") != variablesMap.end())
    {
        cout << CommandlineParams::getHelpInfo() << endl;
        quick_exit(0);
    }
}

void ems::utils::configurationInit()
{
    auto& variablesMap = CommandlineParams::getVariablesMap();
    auto result = variablesMap.find("configure");
    BOOST_ASSERT(result != variablesMap.end());
    const string cfgPath = result->second.as<string>();
    YamlcppWrapper::init(cfgPath + "/config.yaml");
}

void ems::utils::configurationInitFromEnv()
{
try
{
    const string envName{ "PACEIC_EMS_SERVER" };
    const char* envValue = std::getenv(envName.c_str());
    if(!envValue){
        throw std::runtime_error("env variables 'PACEIC_EMS_SERVER' not set");
    }
    string filename(envValue);
    filename += "/etc/config.yaml";
    YamlcppWrapper::init(filename);
}
catch(const std::exception& e){
    std::cerr << e.what() << '\n';
    std::quick_exit(-1);
}
}

void ems::utils::logInit()
{
    auto& cfgRoot = YamlcppWrapper::getRoot();
    const auto& logCfg = cfgRoot["logger"];
    vector<LogAttr> logAttrs;
    std::transform(logCfg.begin(), logCfg.end(), back_inserter(logAttrs), 
        [](const YAML::Node& attr){
        return make_tuple(
            attr["name"].as<string>(),
            attr["priority"].as<string>()
        );
    });
    const string rootPath = cfgRoot["global"]["rootPath"].as<string>();
    const string logPath = cfgRoot["global"]["logPath"].as<string>();
    Log4cppWrapper::init(logAttrs, rootPath + logPath);
}
