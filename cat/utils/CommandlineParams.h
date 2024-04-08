#include "boost/program_options.hpp"
#include <utility>
#include <string>

namespace commandline_params
{
using namespace std;
using namespace boost::program_options;

class Object
{
public:
    Object() = delete;
    static void init(int argc, char** argv);
    static variables_map& getVariablesMap(){ return variablesMap_; }
    static string getHelpInfo();
private:
    static options_description optionsDescription_;
    static variables_map variablesMap_;
};

}
