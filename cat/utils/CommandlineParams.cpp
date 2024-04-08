#include "CommandlineParams.h"
#include <stdlib.h>
#include <sstream>
using namespace commandline_params;

options_description Object::optionsDescription_("命令行参数");
variables_map Object::variablesMap_;

/**
 * 仅在main()处调用一次
*/
void commandline_params::Object::init(int argc, char **argv)
{
    optionsDescription_.add_options()
        ("daemon,d", "后台运行")
        ("configure,c", value<string>(), "配置文件所在路径")
        ("help,h", "帮助信息");
    
    auto parsedOptions = parse_command_line(argc,
        argv, optionsDescription_);
    store(parsedOptions, variablesMap_);
    notify(variablesMap_);
}

string commandline_params::Object::getHelpInfo()
{
    ostringstream oss;
    oss << optionsDescription_;
    return oss.str();
}
