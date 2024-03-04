#include"yuan_thread.hpp"
//#include"yuan_log.cpp"
#include<pthread.h>
#include"yuan_log.hpp"


namespace yuan
{
    /********************************************** Semaphore ******************************************************/
    Semaphore::Semaphore(uint32_t count)
    {
        //0，表示使用线程间共享的信号量。另外一种类型是 SEM_PRIVATE，表示信号量只能由创建它的进程使用，不同进程间不共享。
        if(sem_init(&m_semaphore,0,count))
        {
            throw std::logic_error("sem_init error");
        }
    }
    Semaphore::~Semaphore()
    {
        sem_destroy(&m_semaphore);  
    }

    void Semaphore::wait()
    {
        if(sem_wait(&m_semaphore))
        {
            throw std::logic_error("sem_wait error");
        }
    }
    void Semaphore::notify()
    {
        if(sem_post(&m_semaphore))
        {
            throw std::logic_error("sem_post error");
        }
    }
    /********************************************** Thread ******************************************************/

    //用于指示变量具有线程局部存储（TLS）的语义。这意味着每个线程都会拥有该变量的一份独立实例，每个线程对该变量的修改不会影响其他线程对同一变量的访问。
    static thread_local Thread* t_thread = nullptr;
    static thread_local std::string t_thread_name = "UNKNOW";
    static yuan::Logger::ptr g_logger1 = YUAN_LOG_NAME("system");

    Thread* Thread::GetThis()
    {
        return t_thread;
    }
    const std::string& Thread::GetName()//给日志库使用
    {
        return t_thread_name;
    }

    void Thread::SetName(const std::string& name)
    {
        if(t_thread)
        {
            t_thread->m_name = name;
        }
        t_thread_name = name;
    }

    Thread::Thread(std::function<void()> cb,const std::string& name):m_cb(cb),m_name(name)
    {
        if(name.empty())
        {
            m_name = "UNKNOW";
        }
        int rt = pthread_create(&m_thread,nullptr,&Thread::run,this);
        if(rt)
        {
            YUAN_LOG_ERROR(g_logger1) << "pthread_create thread fail,rt=" << rt << " name=" << name;
            throw std::logic_error("pthread_create error");
        }
        m_semaphore.wait();
    }
    Thread::~Thread()
    {
        if(m_thread)
        {
            //将m_thread设为分离状态，当该线程终止时，系统会自动释放占用的资源
            pthread_detach(m_thread);
        }
    }

    void Thread::join()
    {
        if(m_thread)
        {
            int rt = pthread_join(m_thread,nullptr);
            if(rt)
            {
                YUAN_LOG_ERROR(g_logger1) << "pthread_join thread fail,rt=" << rt << " m_name= " << m_name;
                throw std::logic_error("pthread_join error");
            }
            m_thread = 0;
        }
    }

    void* Thread::run(void* arg)
    {
        Thread* thread = (Thread*)arg;              //将传进来的参数转化为Thread类型的指针，这样就能访问Thread类的成员变量和方法
        t_thread = thread;          //将当前线程对象的成员变量t_thread(thread_local Thread*类型)设置为传入的Thread对象，这样在其他地方就可以使用 t_thread 来操作当前线程对象
        t_thread_name = thread->m_name;
        thread->m_id = yuan::GetThreadId();
        //修改线程名称，接收的子符数最大为16
        pthread_setname_np(pthread_self(),thread->m_name.substr(0,15).c_str());

        std::function<void()> cb;
        //将线程对象中的回调函数 m_cb 与 cb 进行交换，这样 cb 就持有了线程对象中的回调函数，这么做的好处是避免了潜在的资源拷贝，提高了效率。
        cb.swap(thread->m_cb);

        //run被设为静态函数，要指明该对象
        thread->m_semaphore.notify();
        //m_semaphore.notify();
        cb();
        return 0;
    }


}