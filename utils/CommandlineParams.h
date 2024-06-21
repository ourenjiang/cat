#pragma once
#include "boost/program_options.hpp"
#include <utility>
#include <string>

namespace ems
{
using namespace std;
using namespace boost::program_options;

class CommandlineParams
{
public:
    CommandlineParams() = delete;
    static void init(int argc, char** argv);
    static variables_map& getVariablesMap(){ return variablesMap_; }
    static string getHelpInfo();
private:
    inline static options_description optionsDescription_{"命令行参数"};
    inline static variables_map variablesMap_;
};

}//namespace ems
