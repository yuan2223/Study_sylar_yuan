#include "yuan_scheduler.hpp"
// #include"yuan_log.hpp"
#include "yuan_macro.hpp"
#include "yuan_thread.cpp"
#include "yuan_fiber.cpp"

namespace yuan
{
    static yuan::Logger::ptr g_logger2 = YUAN_LOG_NAME("system");

    static thread_local Scheduler *t_scheduler = nullptr;   // 当前线程的调度器,
    static thread_local Fiber *t_scheduler_fiber = nullptr; // 当前线程的调度协程，每个线程独有一份，包括caller线程

    //                 默认         1            true
    Scheduler::Scheduler(size_t threads, bool use_call, const std::string &name) : m_name(name),m_useCaller(use_call)
    {
        YUAN_ASSERT1(threads > 0);
        if (use_call) // 调度线程参与调度
        {
            --threads;
            yuan::Fiber::GetThis(); // 该线程没有主协程的话初始化一个

            YUAN_ASSERT1(GetThis() == nullptr);
            t_scheduler = this;

            m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, false));
            yuan::Thread::SetName(m_name);

            t_scheduler_fiber = m_rootFiber.get();
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
        YUAN_LOG_INFO(g_logger2) << "Scheduler::~Scheduler()";
        YUAN_ASSERT1(m_stopping);
        if (GetThis() == this)
        {
            t_scheduler = nullptr;
        }
    }

    Scheduler *Scheduler::GetThis()
    {
        return t_scheduler;
    }

    Fiber *Scheduler::GetMainFiber()
    {
        return t_scheduler_fiber;
    }

    void Scheduler::start()
    {
        YUAN_LOG_INFO(g_logger2) << "start";
        Mutex::Lock lock(m_mutex);
        if (m_stopping) // 已经启动了
        {
            YUAN_LOG_ERROR(g_logger2) << "Scheduler is stopped";
            return;
        }
        //m_stopping = false;
        YUAN_ASSERT1(m_threads.empty());
        
        // 初始化
        m_threads.resize(m_threadCount);
        for (size_t i = 0; i < m_threadCount; ++i)
        { //      回调函数                     name
            m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i)));
            m_threadIds.push_back(m_threads[i]->getId());
        }
        // if(m_rootFiber)
        // {
        //     m_rootFiber->call();
        //     //m_rootFiber->swapIn();
        //     YUAN_LOG_INFO(g_logger2) << "call out " << m_rootFiber->getState();
        // }
    }

    void Scheduler::stop()
    {
        YUAN_LOG_INFO(g_logger2) << "stop";
        //m_autoStop = true;
        //if (m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM || m_rootFiber->getState() == Fiber::INIT))
        // if (m_rootFiber && m_threadCount == 0 && (m_rootFiber->getState() == Fiber::TERM))
        // {
        //     YUAN_LOG_INFO(g_logger2) << this << " stopping";
        //     m_stopping = true;
        //     if (stopping())
        //     {
        //         return;
        //     }
        // }
        // bool exit_on_this_fiber = false;

        if(stopping())
        {
            return;
        }
        m_stopping = true;

        if (m_useCaller) // use_caller = false
        {
            YUAN_ASSERT1(GetThis() == this);
        }
        else
        {
            YUAN_ASSERT1(GetThis() != this);
        }

        
        // YUAN_LOG_DEBUG(g_logger2) << "m_stopping = true;";
        // 把线程全部唤醒
        for (size_t i = 0; i < m_threadCount; ++i)
        {
            // YUAN_LOG_DEBUG(g_logger2) << "i=" << i;
            tickle();
        }

        if (m_rootFiber)
        {
            tickle();
        }

        if (m_rootFiber)
        {
            // if (!stopping())
            // {
            //     m_rootFiber->resume();
            // }
            m_rootFiber->resume();
            YUAN_LOG_DEBUG(g_logger2) << "m_rootFiber end";
        }

        std::vector<Thread::ptr> thrs;
        {
            MutexType::Lock lock(m_mutex);
            thrs.swap(m_threads);
        }

        for (auto &i : thrs)
        {
            i->join();
        }
    }

    void Scheduler::setThis()
    {
        t_scheduler = this;
    }

    // void Scheduler::run()
    // {
    //     YUAN_LOG_INFO(g_logger2) <<  m_name << "run";
    //     setThis();

    //     //当前线程ID不等于主线程ID
    //     if(yuan::GetThreadId() != m_rootThread)
    //     {
    //         t_scheduler_fiber = Fiber::GetThis().get();
    //     }

    //     Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle,this)));

    //     Fiber::ptr cb_fiber;
    //     ScheduleTask ft;
    //     while(true)
    //     {
    //         ft.reset();
    //         bool tickle_me = false;//是否通知其他线程进行任务调度
    //         bool is_activate = false;
    //         {
    //             MutexType::Lock lock(m_mutex);

    //             auto it = m_fibers.begin(); //std::list<ScheduleTask> m_fibers; //协程的消息队列,计划要执行的协程
    //             while(it != m_fibers.end())
    //             {
    //                 //指定了调度线程，但不是在当前线程上调度
    //                 if(it->thread != -1 && it->thread != yuan::GetThreadId())
    //                 {
    //                     ++it;
    //                     tickle_me = true;   //通知其他线程
    //                     continue;
    //                 }
    //                 YUAN_ASSERT1(it->fiber || it->cb);
    //                 if(it->fiber && it->fiber->getState() == Fiber::EXEC)
    //                 {
    //                     ++it;
    //                     continue;
    //                 }
    //                 ft = *it;   //把这个任务赋值给ft
    //                 m_fibers.erase(it++);//从任务队列中删除这个任务
    //                 ++m_activeThreadCount;
    //                 is_activate = true;
    //                 break;
    //             }
    //             tickle_me |= it != m_fibers.end();
    //         }
    //         if(tickle_me)
    //         {
    //             tickle();
    //         }
    //         // 是协程  &&   这个任务里面有个协程
    //         if(ft.fiber && (ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT))
    //         {

    //             ft.fiber->swapIn();     //切换到ft里面的协程执行
    //             --m_activeThreadCount;  //协程执行完

    //             if(ft.fiber->getState() == Fiber::READY)
    //             {
    //                 schedule(ft.fiber); //把这个协程放到任务队列中
    //             }
    //             else if(ft.fiber->getState() != Fiber::TERM && ft.fiber->getState() != Fiber::EXCEPT)
    //             {
    //                 ft.fiber->m_state = Fiber::HOLD;
    //             }
    //             ft.reset();
    //         }
    //         else if(ft.cb) //是回调函数
    //         {
    //             if(cb_fiber)
    //             {
    //                 cb_fiber->reset(ft.cb);
    //             }
    //             else
    //             {
    //                 cb_fiber.reset(new Fiber(ft.cb));
    //             }

    //             ft.reset();

    //             cb_fiber->swapIn();
    //             --m_activeThreadCount;

    //             if(cb_fiber->getState() == Fiber::READY)
    //             {
    //                 schedule(cb_fiber);
    //                 cb_fiber.reset();
    //             }
    //             else if(cb_fiber->getState() == Fiber::EXCEPT || cb_fiber->getState() == Fiber::TERM)
    //             {
    //                 cb_fiber->reset(nullptr);
    //             }
    //             else
    //             {
    //                 cb_fiber->m_state = Fiber::HOLD;
    //                 cb_fiber.reset();
    //             }
    //         }
    //         else    //没有任务
    //         {
    //             if(is_activate)
    //             {
    //                 --m_activeThreadCount;
    //                 continue;
    //             }
    //             if(idle_fiber->getState() == Fiber::TERM)
    //             {
    //                 YUAN_LOG_INFO(g_logger2) << "idle fiber term";              //////////////////////////////////////////////////////////////////
    //                 break;
    //             }

    //             ++m_idleThreadCount;
    //             idle_fiber->swapIn();
    //              --m_idleThreadCount;
    //             if(idle_fiber->getState() == Fiber::TERM && idle_fiber->getState() == Fiber::EXCEPT)
    //             {
    //                 idle_fiber->m_state = Fiber::HOLD;
    //             }

    //         }
    //     }
    // }

    // git
    void Scheduler::run()
    {
        YUAN_LOG_INFO(g_logger2) << "Scheduler run begin";
        setThis();
        if (yuan::GetThreadId() != m_rootThread) //当前的线程不是调度器所在线程
        {
            t_scheduler_fiber = Fiber::GetThis().get();
        }
        YUAN_LOG_INFO(g_logger2) << "创建任务协程";
        Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
        Fiber::ptr cb_fiber;

        ScheduleTask ft;
        while (true)
        {
            ft.reset();
            bool tickle_me = false;
            //bool is_active = false;
            {
                MutexType::Lock lock(m_mutex);
                auto it = m_fibers.begin();
                while (it != m_fibers.end())
                {
                    //该任务指定了调度的线程，但不是在当前线程上调度
                    if (it->thread != -1 && it->thread != yuan::GetThreadId())
                    {
                        ++it;
                        tickle_me = true;
                        continue;
                    }

                    YUAN_ASSERT1(it->fiber || it->cb);
                    if (it->fiber)
                    {
                        YUAN_ASSERT1(it->fiber->getState() == Fiber::READY);
                    }

                    //当前调度线程找到一个任务，开始调度
                    ft = *it;
                    m_fibers.erase(it++);
                    ++m_activeThreadCount;
                    break;
                }
                tickle_me |= (it != m_fibers.end());
            }

            if (tickle_me)
            {
                tickle();
            }

            if (ft.fiber)
            {
                //resume返回时，协程要么执行完了，要么yield了，这个任务就算完成了，活跃线程数减一
                ft.fiber->resume();
                --m_activeThreadCount;
                ft.reset();
            }
            else if (ft.cb)
            {
                if (cb_fiber)
                {
                    cb_fiber->reset(ft.cb);
                }
                else
                {
                    cb_fiber.reset(new Fiber(ft.cb));
                }
                ft.reset();
                cb_fiber->resume();
                --m_activeThreadCount;
                cb_fiber.reset();
            }
            else
            {
                //任务队列为空，调度协程进入idle，idle协程会不停地resume/yield
                //调度器停止，idle协程才会停止
                if (idle_fiber->getState() == Fiber::TERM)
                { //
                    YUAN_LOG_INFO(g_logger) << "idle fiber term";
                    break;
                }

                ++m_idleThreadCount;
                //idle_fiber->swapIn();
                idle_fiber->resume();
                --m_idleThreadCount;
            }
        }
        YUAN_LOG_INFO(g_logger2) << "Scheduler run end";
    }

    void Scheduler::tickle()

    {
        YUAN_LOG_INFO(g_logger2) << "tickle";
    }

    bool Scheduler::stopping()
    {
        MutexType::Lock lock(m_mutex);
        return  m_stopping && m_fibers.empty() && m_activeThreadCount == 0;
    }

    void Scheduler::idle()
    {
        YUAN_LOG_INFO(g_logger2) << "idle";
        while (!stopping())
        {
            yuan::Fiber::GetThis()->yield();
        }
    }
}