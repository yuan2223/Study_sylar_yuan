#include "yuan_until.hpp"
#include<sys/syscall.h>
#include<unistd.h>

namespace yuan
{
   //获取线程ID
    pid_t GetThreadId()
    {
        return syscall(SYS_gettid);
    }

    //获取协程ID
    uint32_t GetFiberId()
    {
        return 1;
    }
}