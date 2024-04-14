#include "myself/base/Thread.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

namespace myself
{
namespace CurrentThread
{
__thread int t_cachedTid;
__thread char t_tidString[32];

namespace detail
{
void cacheTid()
{
    if(t_cachedTid == 0)
    {
        t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        int __attribute__((unused)) n = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
        // assert(n == 6); 格式化时，末尾留了一个空格。实际测试得到一个6位数的tid，那么整个格式化字符串就是7个字符了，不符？
    }
}
}//namespace detail

int tid()
{
    if(t_cachedTid == 0)
    {
        detail::cacheTid();
    }
    return t_cachedTid;
}

}//namespace CurrentThread
}//namespace myself
