#include "CommandlineParams.h"
#include <stdlib.h>
#include <sstream>

using namespace ems;

/**
 * 仅在main()处调用一次
*/
void CommandlineParams::init(int argc, char **argv)
{
    optionsDescription_.add_options()
        ("configure,c", value<string>(), "配置文件所在路径")
        ("help,h", "帮助信息");
    
    auto parsedOptions = parse_command_line(argc,
        argv, optionsDescription_);
    store(parsedOptions, variablesMap_);
    notify(variablesMap_);
}

string CommandlineParams::getHelpInfo()
{
    ostringstream oss;
    oss << optionsDescription_;
    return oss.str();
}
