//#include "yuan_log.cpp"
#include "yuan_thread.cpp"

yuan::Logger::ptr test_logger = YUAN_LOG_ROOT();
//volatile
int count = 0;
yuan::RWMutex s_mutex;
void func1()
{
    YUAN_LOG_INFO(test_logger) << "name: " << yuan::Thread::GetName()
                            << " this.name: " <<yuan::Thread::GetThis()->getName()
                            << " id: " << yuan::GetThreadId()
                            << " this.id: " << yuan::Thread::GetThis()->getId();
    for(int i = 0;i<100000;++i)
    {
        yuan::RWMutex::WriteLock lock(s_mutex);
        count++;
    }
}
//g++ test_thread.cpp -lyaml-cpp -pthread -o test_thread
int main()
{
    YUAN_LOG_INFO(test_logger) << "thread test begin";
    std::vector<yuan::Thread::ptr> thv;
    for(int i = 0;i<5;++i)
    {
        yuan::Thread::ptr t(new yuan::Thread(func1,"name_" + std::to_string(i)));
        thv.push_back(t);
    }

    for(int i = 0;i<5;++i)
    {
        thv[i]->join();
    }
    YUAN_LOG_INFO(test_logger) << "thread test end";
    YUAN_LOG_INFO(test_logger) << "count="<< count;
    return 0;
}