#ifndef __YUAN_UNTIL_HPP__
#define __YUAN_UNTIL_HPP__

#include<cstdint>
#include<sys/syscall.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdint.h>
#include<vector>
#include<string>


namespace yuan
{
    //获取线程ID
    pid_t GetThreadId();

    //获取协程ID
    uint32_t GetFiberId();

    //size: 最大获得多少层  skip: 前面掠过的层数  一般第一层是自己，掠过
    void Backtrace(std::vector<std::string&>& bt,int size = 64,int skip = 1);

    std::string BacktraceToString(int size = 64,int skip = 2,const std::string prefix = "");

    uint64_t GetCurrentMS();        //毫秒
    uint64_t GetCurrentUS();        //微秒
}


#endif //__YUAN_UTIL_H__