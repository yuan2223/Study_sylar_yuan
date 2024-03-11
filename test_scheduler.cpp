#include "yuan_log.cpp"
#include "yuan_scheduler.cpp"

static yuan::Logger::ptr g_lpgger = YUAN_LOG_ROOT();

void test_fiber()
{
    YUAN_LOG_INFO(g_lpgger) << "run in test_fiber";
    static int s_count = 5;
    YUAN_LOG_INFO(g_lpgger) << "s_count=" << s_count;
    sleep(1);
    // if (--s_count)
    // {
    //     yuan::Scheduler::GetThis()->schedule(&test_fiber);
    // }
    
}

//  g++ test_scheduler.cpp -lyaml-cpp -pthread -o test_scheduler

int main()
{
    YUAN_LOG_INFO(g_lpgger) << "main";
    yuan::Scheduler sc(3,true,"test");
    sc.start();
    YUAN_LOG_INFO(g_lpgger) << "scheduler";
    sc.schedule(&test_fiber);
    sc.stop();
    YUAN_LOG_INFO(g_lpgger) << "over";
    return 0;
}