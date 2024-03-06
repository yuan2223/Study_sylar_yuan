#include "yuan_fiber.cpp"
#include "yuan_thread.cpp"
#include "yuan_log.cpp"
#include<vector>

yuan::Logger::ptr g_logger = YUAN_LOG_ROOT();

void run_in_fiber()
{
    YUAN_LOG_INFO(g_logger) << "run_in_fiber begin";
    //yuan::Fiber::GetThis()->swapOut();  //交出当前协程，切换到主协程
    yuan::Fiber::YieldToHold();

    YUAN_LOG_INFO(g_logger) << "run_in_fiber end";
    yuan::Fiber::YieldToHold();
}

void test_fiber()
{
    //std::cout << "===========================================================" << std::endl;
    {
        yuan::Fiber::GetThis();     //创建当前线程的主协程
        YUAN_LOG_INFO(g_logger) << "main begin";
        yuan::Fiber::ptr fiber(new yuan::Fiber(run_in_fiber)); //主协程下创建子协程
        fiber->swapIn();        //切换到子协程下运行 run_in_fiber函数
        YUAN_LOG_INFO(g_logger) << "main after swapIn";
        fiber->swapIn();
        YUAN_LOG_INFO(g_logger) << "main after end";
        fiber->swapIn();
    }
    YUAN_LOG_INFO(g_logger) << "main after end2";
}

int main()
{
    yuan::Thread::SetName("main");
    std::vector<yuan::Thread::ptr> ths;
    for(int i = 0;i<3;++i)
    {
        ths.push_back(yuan::Thread::ptr(new yuan::Thread(&test_fiber,"name"+std::to_string(i))));
    }
    for(int i = 0;i<3;++i)
    {
        ths[i]->join();
    }
    return 0;
}