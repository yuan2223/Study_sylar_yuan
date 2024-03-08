#ifndef __YUAN_SCHEDULER_HPP__
#define __YUAN_SCHEDULER_HPP__

#include<memory>
#include<vector>
#include<list>
#include"yuan_fiber.hpp"
#include "yuan_thread.hpp"

namespace yuan
{
    class Scheduler
    {
    public:
        typedef std::shared_ptr<Scheduler> ptr;
        typedef Mutex MutexType;
        //           线程数                 是否将当前线程加入到调度器中
        Scheduler(size_t threads = 1,bool use_call = true,const std::string& name = "");
        virtual ~Scheduler();

        const std::string& getName() const {return m_name;}

        static Scheduler* GetThis();
        static Fiber* GetMainFiber();

        void start();
        void stop();

        template<class FiberOrCb>
        void schedule(FiberOrCb fc,int thread = -1)
        {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                need_tickle = scheduleNoLock(fc,thread);
            }
            if(need_tickle)
            {
                tickle();
            }
        }

        template<class InputIterator>
        void schedule(InputIterator begin,InputIterator end)
        {
            bool need_tickle = false;
            {
                MutexType::Lock lock(m_mutex);
                while(begin != end)
                {
                    need_tickle = scheduleNoLock(&*begin) || need_tickle;
                }
            }
            if(need_tickle)
            {
                tickle();
            }
        }
    
    protected:
        virtual void tickle();
        virtual bool stopping();
        virtual void idle();

        void setThis();
        void run();

    private:
        template<class FiberOrCb>
        bool scheduleNoLock(FiberOrCb fc,int thread)
        {
            bool need_tickle = m_fibers.empty();//计划要执行的协程是否为空
            FiberAndThread ft(fc,thread);
            if(ft.fiber || ft.cb)
            {
                m_fibers.push_back(ft);
            }
            return need_tickle;
        }

    private:
        struct ScheduleTask
        {
            Fiber::ptr fiber;
            std::function<void()> cb;
            int thread;//线程id
            //传入一个协程 线程id ，这个协程在这个线程上运行
            ScheduleTask(Fiber::ptr f,int thr):fiber(f),thread(thr) {}
           
            ScheduleTask(Fiber::ptr* f,int thr):thread(thr)
            {
                fiber.swap(*f);
            }

            ScheduleTask(std::function<void()> f,int thr) : cb(f),thread(thr) {}

            ScheduleTask(std::function<void()>* f,int thr) :thread(thr)
            {
                cb.swap(*f);
            }

            //没有默认构造函数，分配的对象无法初始化
            ScheduleTask():thread(-1){}//不指定任何线程

            void reset()
            {
                fiber = nullptr;
                cb = nullptr;
                thread = -1;
            }         
        };
    private:
        MutexType m_mutex;
        std::vector<Thread::ptr> m_threads;
        std::list<ScheduleTask> m_fibers; //协程的消息队列,计划要执行的协程
        Fiber::ptr m_rootFiber;
        std::string m_name;
    protected:
        std::vector<int> m_threadIds;       //所有的线程ID
        std::atomic<size_t> m_threadCount = {0};           //线程数量
        std::atomic<size_t> m_activeThreadCount = {0};     //活跃线程数量
        size_t m_idleThreadCount = 0;       //空闲线程数量
        bool m_stopping = true;             //还没有启动
        bool m_autoStop = false;;
        int m_rootThread = 0;               //主线程的ID，指的是scheduler线程
    };
}
#endif;