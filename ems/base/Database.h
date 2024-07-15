#pragma once
#include <string>
#include "sqlite_modern_cpp.h"

namespace ems
{
namespace base
{
using namespace std;

class Database
{
public:
    static sqlite::database open(const string& filepath);
};
}//namespace base
}//namespace ems
