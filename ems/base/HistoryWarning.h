#pragma once
#include <utility>
#include <string>

namespace ems
{
namespace base
{
using namespace std;

class HistoryWarning
{
public:
    static void insertRecord(const string content, const string level, const string deviceName, const string createTime, const string action, const string processed);
};

}//namespace base
}//namespace ems
