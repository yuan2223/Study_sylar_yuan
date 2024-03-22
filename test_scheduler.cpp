#include "yuan_log.cpp"
#include "yuan_scheduler.cpp"

static yuan::Logger::ptr g_lpgger = YUAN_LOG_ROOT();

void test_fiber1()
{
    YUAN_LOG_INFO(g_lpgger) << "test_fiber1 begin";
    
    //yuan::Scheduler::GetThis()->schedule(yuan::Fiber::GetThis());

    // YUAN_LOG_INFO(g_lpgger) << "before test_fiber1 yield";
    // yuan::Fiber::GetThis()->yield();
    // YUAN_LOG_INFO(g_lpgger) << "after test_fiber1 yield";

    // YUAN_LOG_INFO(g_lpgger) << "test_fiber1 end";
}

void test_fiber2()
{
    YUAN_LOG_INFO(g_lpgger) << "test_fiber2 begin";
    sleep(2);
    YUAN_LOG_INFO(g_lpgger) << "test_fiber2 end";
}

void test_fiber3()
{
    YUAN_LOG_INFO(g_lpgger) << "test_fiber3333";
}
//  g++ test_scheduler.cpp -lyaml-cpp -pthread -o test_scheduler

int main()
{
    yuan::Thread::SetName("main");
    YUAN_LOG_INFO(g_lpgger) << "main";
    // yuan::Scheduler sc(2,true,"test");
    yuan::Scheduler sc;
    
    YUAN_LOG_INFO(g_lpgger) << "scheduler";

    
    sc.schedule(&test_fiber1);//添加调度任务
    sc.schedule(&test_fiber2);
    // yuan::Fiber::ptr ff(new yuan::Fiber(&test_fiber3));
    // sc.schedule(ff);

    sc.start();

    sc.schedule(test_fiber3);
    //如果使用了call线程，调度器依赖stop方法来执行call线程的调度协程
    sc.stop();
    YUAN_LOG_INFO(g_lpgger) << "over";
    return 0;
}