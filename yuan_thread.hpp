#ifndef YUAN_THREAD_HPP__
#define YUAN_THREAD_HPP__
//线程作为协程的容器，协程在线程上跑
#include<thread>
#include<functional>
#include<memory>
#include<semaphore.h>
#include<stdint.h>
#include"yuan_log.hpp"


namespace yuan
{
    /********************************************** Semaphore ******************************************************/
    //信号量
    class Semaphore
    {
    public:
        Semaphore(uint32_t count = 0);
        ~Semaphore();

        void wait();
        void notify();
    private:
        Semaphore(const Semaphore&) = delete;
        Semaphore(const Semaphore&&) = delete;
        Semaphore& operator=(const Semaphore&) = delete;
    private:
        sem_t m_semaphore;

    };

    /********************************************** ScopedLockImpl ******************************************************/
    template<class T>
    struct ScopedLockImpl
    {
    public:
        //构造函数加锁
        ScopedLockImpl(T& mutex):m_mutex(mutex)
        {
            m_mutex.lock();
            m_locked = true;
        }
        //析构函数解锁
        ~ScopedLockImpl()
        {
            unlock();
        }

        void lock()
        {
            if(!m_locked)//没有加锁
            {
                m_mutex.lock();
                m_locked = true;
            }
        }
        void unlock()
        {
            if(m_locked)//已经加锁了
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };

    template<class T>
    struct ReadScopedLockImpl
    {
    public:
        //构造函数加锁
        ReadScopedLockImpl(T& mutex):m_mutex(mutex)
        {
            m_mutex.rdlock();
            m_locked = true;
        }
        //析构函数解锁
        ~ReadScopedLockImpl()
        {
            unlock();
        }

        void lock()
        {
            if(!m_locked)//没有加锁
            {
                m_mutex.rdlock();
                m_locked = true;
            }
        }
        void unlock()
        {
            if(m_locked)//已经加锁了
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };
    template<class T>
    struct WriteScopedLockImpl
    {
    public:
        //构造函数加锁
        WriteScopedLockImpl(T& mutex):m_mutex(mutex)
        {
            m_mutex.wrlock();
            m_locked = true;
        }
        //析构函数解锁
        ~WriteScopedLockImpl()
        {
            unlock();
        }

        void lock()
        {
            if(!m_locked)//没有加锁
            {
                m_mutex.wrlock();
                m_locked = true;
            }
        }
        void unlock()
        {
            if(m_locked)//已经加锁了
            {
                m_mutex.unlock();
                m_locked = false;
            }
        }
    private:
        T& m_mutex;
        bool m_locked;
    };

    /********************************************** RWMutex ******************************************************/
    //读写锁
    //允许多个线程同时对共享资源进行读操作，但是只允许一个线程进行写操作。
    //这种锁机制在读多写少的场景下可以提供更好的性能，因为多个线程可以同时读取共享资源，而不必相互等待，但写操作需要独占资源，所以只允许一个线程进行写操作。
    class RWMutex
    {
    public:
        typedef ReadScopedLockImpl<RWMutex> ReadLock;
        typedef WriteScopedLockImpl<RWMutex> WriteLock;
        RWMutex()
        {
            pthread_rwlock_init(&m_lock,nullptr);
        }
        ~RWMutex()
        {
            pthread_rwlock_destroy(&m_lock);
        }
        //使用读模式锁定读写锁 m_lock, 但是当有线程持有写锁时，读锁请求会被阻塞，直到写锁被释放
        void rdlock()
        {
            pthread_rwlock_rdlock(&m_lock);
        }
        //使用写模式锁定读写锁 m_lock。当某个线程持有写锁时，其他线程无法获取读或写锁，直到写锁被释放
        void wrlock()
        {
            pthread_rwlock_wrlock(&m_lock);
        }
        //用于释放之前获取的读或写锁 m_lock。允许其他线程获取这个锁以继续执行
        void unlock()
        {
            pthread_rwlock_unlock(&m_lock);
        }
    private:
        pthread_rwlock_t m_lock;//读写锁的变量，通过该变量控制多线程对共享资源的并发访问
    };

    /********************************************** Thread ******************************************************/
    class Thread
    {
    public:
        typedef std::shared_ptr<Thread> ptr;

        Thread(std::function<void()> cb,const std::string& name);
        ~Thread();

        pid_t getId() const {return m_id;}
        const std::string getName() const {return m_name;}

        void join();

        static Thread* GetThis();
        static const std::string& GetName();//给日志库使用
        static void SetName(const std::string& name);

    private:
        Thread(const Thread&) = delete;
        Thread(const Thread&&) = delete;
        Thread operator=(const Thread&) = delete;

        static void* run(void* arg);

    private:
        pid_t m_id = -1;
        pthread_t m_thread = 0;
        std::function<void()> m_cb;
        std::string m_name;
        Semaphore m_semaphore;
    };
}


#endif