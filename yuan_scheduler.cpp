#include"yuan_scheduler.hpp"
#include"yuan_log.hpp"
#include "yuan_macro.hpp"

namespace yuan
{
    static yuan::Logger::ptr g_logger = YUAN_LOG_NAME("system");

    static thread_local Scheduler* t_scheduler = nullptr;
    static thread_local Fiber* t_fiber = nullptr;
    Scheduler::Scheduler(size_t threads,bool use_call,const std::string& name):m_name(name)
    {
        YUAN_ASSERT1(threads > 0);
        if(use_call)//调度线程参与调度
        {
            yuan::Fiber::GetThis();//该线程没有主协程的话初始化一个
            --threads;

            YUAN_ASSERT1(GetThis == nullptr);
            t_scheduler = this;

            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run,this)));
            yuan::Thread::SetName(m_name);
            t_fiber = m_rootFiber.get();
            m_rootThread = yuan::GetThreadId();
            m_threadIds.push_back(m_rootThread);
        }
        else
        {
            m_rootThread = -1;
        }
        m_threadCount = threads;
    }

    Scheduler::~Scheduler()
    {
        YUAN_ASSERT1(m_stopping);
        if(GetThis() == this)
        {
            t_scheduler = nullptr;
        }
    }

    Scheduler* Scheduler::GetThis()
    {
        return t_scheduler;
    }

    Fiber* Scheduler::GetMainFiber()
    {
        return t_fiber;
    }

    void Scheduler::start()
    {
        Mutex::Lock lock(m_mutex);
        if(!m_stopping) //已经启动了
        {
            return;
        }
        m_stopping = false;
        YUAN_ASSERT1(m_threads.empty());    
        m_threads.resize(m_threadCount);
        for(size_t i = 0;i<m_threadCount;++i)
        {                                   //      回调函数                     name
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run,this),m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
    }

    void Scheduler::stop()
    {
        m_autoStop = true;
        if(m_rootFiber && m_threadCount == 0
                       && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT))
        {
            YUAN_LOG_INFO(g_logger) << this << "stopping";
            m_stopping = true;
            if(stopping())
            {
                return;
            }
        }
        bool exit_on_this_fiber = false;
        if(m_rootThread != -1)//use_caller = false
        {
            YUAN_ASSERT1(GetThis() == this);
        }
        else
        {
            YUAN_ASSERT1(GetThis() != this);
        }
        m_stopping = true;
        //把线程全部唤醒
        for(size_t i = 0;i < m_threadCount;++i)
        {
            tickle();
        }
        if(m_rootFiber)
        {
            tickle();
        }
        if(stopping())
        {
            return;
        }
    }

    void Scheduler::setThis()
    {
        t_scheduler = this;
    }

    void Scheduler::run()
    {
        setThis();
        //当前线程ID不等于主线程ID
        if(yuan::GetThreadId() != m_rootThread)
        {
            t_fiber = Fiber::GetThis().get();
        }
        
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle,this)));

        Fiber::ptr cb_fiber;
        ScheduleTask ft;
        while(true)
        {
            ft.reset();
            bool tickle_me = false;//是否通知其他线程进行任务调度
            {
                Mutex::Lock lock(m_mutex);

                auto it = m_fibers.begin(); //std::list<ScheduleTask> m_fibers; //协程的消息队列,计划要执行的协程
                while(it != m_fibers.end())
                {
                    //指定了调度线程，但不是在当前线程上调度
                    if(it->thread != -1 && it->thread != yuan::GetThreadId())
                    {
                        ++it;
                        tickle_me = true;   //通知其他线程
                        continue;
                    }
                    YUAN_ASSERT1(it->fiber || it->cb);
                    if(it->fiber && it->fiber->getState() == Fiber::EXCEPT)
                    {
                        ++it;
                        continue;
                    }
                    ft = *it;   //把这个任务赋值给ft
                    m_fibers.erase(it);//从任务队列中删除这个任务
                }
            }
            if(tickle_me)
            {
                tickle();
            }
            // 是协程  &&   这个任务里面有个协程
            if(ft.fiber && (ft.fiber->getState() != Fiber::TERM || ft.fiber->getState() != Fiber::EXCEPT))
            {
                ++m_activeThreadCount;
                ft.fiber->swapIn();     //切换到ft里面的协程执行
                --m_activeThreadCount;  //协程执行完

                if(ft.fiber->getState() == Fiber::READY)
                {
                    schedule(ft.fiber); //把这个协程放到任务队列中
                }
                else if(ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
                {
                    //私有成员不可访问ft.fiber->m_state = Fiber::HOLD;
                }
                ft.reset();

            }
            else if(ft.cb) //是回调函数
            {
                if(cb_fiber)
                {
                    cb_fiber->reset(ft.cb);
                    //cb_fiber->reset(&ft.cb);
                }
                else
                {
                    cb_fiber.reset(new Fiber(ft.cb));
                }

                ft.reset();
                ++m_activeThreadCount;
                cb_fiber->swapIn();
                --m_activeThreadCount;

                if(cb_fiber->getState() == Fiber::READY)
                {
                    schedule(cb_fiber);
                    cb_fiber.reset();
                }
                else if(cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM)
                {
                    cb_fiber->reset(nullptr);
                }
                else
                {
                    //私有成员不可访问cb_fiber->m_state = Fiber::HOLD;
                    cb_fiber.reset();
                }
            }
            else    //没有任务
            {   
                if(idle_fiber->getState() == Fiber::TERM)
                {
                    break;
                }
                ++m_idleThreadCount;
                idle_fiber->swapIn();
                 --m_idleThreadCount;
                if(idle_fiber->getState() == Fiber::TERM || idle_fiber->getState() == Fiber::EXCEPT)
                {
                    //私有成员不可访问idle_fiber->m_state = Fiber::HOLD;
                }
               
            }
        }
    }
}