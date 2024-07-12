#pragma once
#include <assert.h>
#include <unistd.h>

namespace ems
{
namespace miscellaneous
{
using namespace std;

// 强制ROOT权限
class AssertRootPriority
{
public:
    AssertRootPriority(){
        const uid_t rootUid{};// Root用法的UID总是为0
        assert(getuid() == rootUid);
    }
};

}////namespace miscellaneous
}//namespace ems
