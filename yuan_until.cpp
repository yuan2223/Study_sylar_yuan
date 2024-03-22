#include "yuan_until.hpp"
#include <sstream>
#include<iostream>
#include<execinfo.h>
#include<sys/time.h>
#include"yuan_fiber.hpp"



namespace yuan
{
    // yuan::Logger::ptr g_logger = YUAN_LOG_NAME("system");
   //获取线程ID
    pid_t GetThreadId()
    {
        return syscall(SYS_gettid);
    }

    //获取协程ID
    uint32_t GetFiberId()
    {
        return yuan::Fiber::GetFiberId();
    }

    //size: 最大获得多少层  skip: 前面掠过的层数  一般第一层是自己，掠过
    void Backtrace(std::vector<std::string>& bt,int size,int skip)
    {
        void** array = (void**)malloc((sizeof(void**) * size));
        //backtrace 函数用于获取当前函数调用栈的信息，存在array指向的指针   返回调用栈信息的数量s
        size_t s = ::backtrace(array,size);

        char** strings = backtrace_symbols(array,s);
        if(strings == NULL)
        {
            // YUAN_LOG_ERROR(g_logger) << "backtrace_symbols error";
            std::cout << "backtrace_symbols error" << std::endl;
            return;
        }

        for(size_t i = skip;i<s;++i)
        {
            bt.push_back(strings[i]);
        }
        free(strings);
        free(array);
    }

    std::string BacktraceToString(int size,int skip,std::string prefix)
    {
        std::vector<std::string> bt;
        Backtrace(bt,size,skip);
        std::stringstream ss;
        for(size_t i = 0;i<bt.size();++i)
        {
            ss << prefix << bt[i] << std::endl;
        }
        return ss.str();
    }

    uint64_t GetCurrentMS()        //毫秒
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec *1000 + tv.tv_usec / 1000;
    }
    uint64_t GetCurrentUS()        //微秒
    {
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec *1000*1000 + tv.tv_usec;
    }
}