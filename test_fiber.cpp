#include "yuan_fiber.hpp"
#include "yuan_thread.hpp"
#include "yuan_log.cpp"
#include "yuan_scheduler.cpp"
#include<vector>

yuan::Logger::ptr g_logger = YUAN_LOG_ROOT();

void run_in_fiber()
{
    YUAN_LOG_INFO(g_logger) << "before run_in_fiber yield";
    yuan::Fiber::GetThis()->yield();
    YUAN_LOG_INFO(g_logger) << "after run_in_fiber yield";
}

void test_fiber()
{
    YUAN_LOG_INFO(g_logger) << "test_fiber begin";

    yuan::Fiber::GetThis();//初始化线程的主协程
    yuan::Fiber::ptr fiber(new yuan::Fiber(&run_in_fiber,0,false));

    YUAN_LOG_INFO(g_logger) << "before resume test_fiber";
    fiber->resume();        //子协程恢复运行
    YUAN_LOG_INFO(g_logger) << "after resume test_fiber";

    fiber->resume();//子协程再次恢复运行

    

    YUAN_LOG_INFO(g_logger) << "run_in_fiber end";
}

// void test11()
// {
//     std::cout << "----------------------------- test ------------------------------" << std::endl;
//     yuan::Fiber::GetThis()->yield();
//     std::cout << "----------------------------- test2 ------------------------------" << std::endl;
// }

int main()
{
    // std::cout << "----------------------------- main ------------------------------" << std::endl;
    // yuan::Fiber::GetThis();
    // yuan::Fiber::ptr fiber(new yuan::Fiber(&test11,0,false));
    // YUAN_LOG_INFO(g_logger) << "before resum";
    // fiber->resume();
    // YUAN_LOG_INFO(g_logger) << "before resum2";
    // fiber->resume();
    // YUAN_LOG_INFO(g_logger) << "after resum2";


    YUAN_LOG_INFO(g_logger) << "main begin";

    yuan::Fiber::GetThis();

    std::vector<yuan::Thread::ptr> thv;
    for(int i = 0;i<3;++i)
    {
        thv.push_back(yuan::Thread::ptr(new yuan::Thread(&test_fiber,"thread_" + std::to_string(i))));
    }

    for(auto& i : thv)
    {
        i->join();
    }
   
    YUAN_LOG_INFO(g_logger) << "main end";
}