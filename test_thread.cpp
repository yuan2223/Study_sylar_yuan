#include "yuan_log.cpp"
#include "yuan_config.cpp"
#include "yuan_thread.cpp"
// #include "yuan_thread.hpp"



yuan::Logger::ptr test_logger = YUAN_LOG_ROOT();
//volatile
int count = 0;
//yuan::RWMutex s_mutex;
yuan::Mutex s_mutex;
void func1()
{
    YUAN_LOG_INFO(test_logger) << "name: " << yuan::Thread::GetName()
                            << " this.name: " <<yuan::Thread::GetThis()->getName()
                            << " id: " << yuan::GetThreadId()
                            << " this.id: " << yuan::Thread::GetThis()->getId();
    for(int i = 0;i<100000;++i)
    {
        //yuan::RWMutex::WriteLock lock(s_mutex);
        yuan::Mutex::Lock lock(s_mutex);
        count++;
    }
}
void func2()
{
    while(1)
    {
        YUAN_LOG_INFO(test_logger) << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    }
}
void func3()
{
    while(2)
    {
        YUAN_LOG_INFO(test_logger) << "==================================================";
    }
}
//g++ test_thread.cpp -lyaml-cpp -pthread -o test_thread
int main()
{
    YUAN_LOG_INFO(test_logger) << "thread test begin";
    YAML::Node root = YAML::LoadFile("/home/bob/桌面/c++_code/study_sylar/log2.yaml");
    yuan::Config::LoadFromYaml(root);
    
    std::vector<yuan::Thread::ptr> thv;
    for(int i = 0;i<2;++i)
    {
        yuan::Thread::ptr t(new yuan::Thread(func2,"name_" + std::to_string(i * 2)));
        yuan::Thread::ptr t2(new yuan::Thread(func3,"name_" + std::to_string(i * 2 + 1)));
        thv.push_back(t);
        thv.push_back(t2);
    }

    for(size_t i = 0;thv.size();++i)
    {
        thv[i]->join();
    }
    YUAN_LOG_INFO(test_logger) << "thread test end";
    YUAN_LOG_INFO(test_logger) << "count="<< count;
    return 0;
}