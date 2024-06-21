#pragma once
#include <string>

namespace ems
{
namespace utils
{

void commandlineParamsInit(int argc, char** argv);
void configurationInit();
void configurationInitFromEnv();
void logInit();

}//namespace utils
}//namespace ems
