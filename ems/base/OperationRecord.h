#pragma once
#include <string>
// #include "Model.h"

namespace ems
{
namespace base
{
using namespace std;

class OperationRecord
{
public:
    static void insertRecord(const string& status, const string& content, const string& type, const string& username);
};

}//namespace base
}//namespace ems
