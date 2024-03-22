#include "yuan_fiber.hpp"
#include "yuan_config.hpp"
#include "yuan_macro.hpp"
#include "yuan_scheduler.hpp"
#include<atomic>

namespace yuan
{
    

    static Logger::ptr g_logger = YUAN_LOG_NAME("system");

    static std::atomic<uint64_t> s_fiber_id {0};
    static std::atomic<uint64_t> s_fiber_count {0}; //统计当前协程数
    static thread_local Fiber* t_fiber = nullptr;   //线程局部变量，当前线程正在运行的协程
    static thread_local Fiber::ptr t_thread_fiber = nullptr;//当前线程的主协程
    
    static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>("fiber.stack_size",128*1024,"fiber stack size");
    class MallocStackAllocator
    {
    public:
        static void* Alloc(size_t size)
        {
            return malloc(size);
        }
        static void Dealloc(void* vp,size_t size)
        {
            free(vp);
        }

    };
    using StackAllocator = MallocStackAllocator;
/******************************************* Fiber************************************************/
    uint64_t Fiber::GetFiberId()
    {
        if(t_fiber)
        {
            return t_fiber->getId();
        }
        return 0;
    }

    //不带参数的构造函数用于初始化的当前线程的协程，构造线程主协程对象
    Fiber::Fiber()
    {
        //m_state = EXEC;
        SetThis(this);
        m_state = EXEC;
        if(getcontext(&m_ctx))
        {
            YUAN_ASSERT2(false,"getcontext");
        }
        ++s_fiber_count;
        m_id = s_fiber_id++;

        YUAN_LOG_INFO(g_logger) << "Fiber::Fiber main  id=" << m_id;
    }
    //每个子协程都有独立的占空间
    Fiber::Fiber(std::function<void()> cb,size_t stacksize,bool run_in_scheduler)
                        :m_id(s_fiber_id++),m_cb(cb),m_runInScheduler(run_in_scheduler)
    {
        ++s_fiber_count;
        m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

        m_stack = StackAllocator::Alloc(m_stacksize);
        if(getcontext(&m_ctx))
        {
            YUAN_ASSERT2(false,"get_context");
        }

        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Fiber::MainFunc, 0);
        // if (!use_caller)
        // {
        //     makecontext(&m_ctx, &Fiber::MainFunc, 0);
        // }
        // else
        // {
        //     makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
        // }

        YUAN_LOG_INFO(g_logger) << "Fiber::Fiber id=" << m_id;
    }
    Fiber::~Fiber()
    {
        --s_fiber_count;
        if(m_stack)//有栈说明是子协程
        {
            YUAN_ASSERT1(m_state == TERM || m_state == EXCEPT);//确保子协程是结束状态
            StackAllocator::Dealloc(m_stack,m_stacksize);
            YUAN_LOG_INFO(g_logger) << "dealloc stack id=" << m_id;
        }
        else
        {
            //没有栈,说明是主协程
            YUAN_ASSERT1(!m_cb);                //主协程没有cb
            YUAN_ASSERT1(m_state == EXEC);   //执行态
            Fiber* cur = t_fiber;
            if(cur == this)
            {
                SetThis(nullptr);
            }
            YUAN_LOG_INFO(g_logger) << "main fiber free,id=" << m_id;
        }
    }
   

    //重置协程状态和入口函数,能重置的协程状态只能为TERM,主要是充分利用内存，一个协程运行完，但内存没有释放，基于这块内存继续初始化
    void Fiber::reset(std::function<void()> cb)
    {
        YUAN_ASSERT1(m_stack);
        //YUAN_ASSERT1(m_state == TERM || m_state == EXCEPT || m_state == INIT);
        YUAN_ASSERT1(m_state == TERM);

        m_cb = cb;
        if(getcontext(&m_ctx))
        {
            YUAN_ASSERT2(false,"getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx,&Fiber::MainFunc,0);
        m_state = READY;
    }

    void Fiber::resume() // 当前线程切换到执行态,当前协程和正在运行的协程切换状态，前者变为RUNNING, 后者为READY
    {
        YUAN_ASSERT1(m_state != TERM && m_state != EXEC);
        SetThis(this);
        m_state = EXEC;

        // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
        if (m_runInScheduler)
        {   //  和当前线程的调度协程交换上下文
            if (swapcontext(&(Scheduler::GetMainFiber()->m_ctx), &m_ctx))
            {
                YUAN_ASSERT2(false, "swapcontext");
            }
        }
        else
        {       //保存主协程的上下文，激活子协程的上下文
            if (swapcontext(&(t_thread_fiber->m_ctx), &m_ctx))
            {
                YUAN_ASSERT2(false, "swapcontext");
            }
        }
    }

    void Fiber::yield()
    {
        // 协程运行完之后会自动yield一次，用于回到主协程，此时状态已为结束状态
        YUAN_ASSERT1(m_state == EXEC || m_state == TERM);
        SetThis(t_thread_fiber.get());
        if (m_state != TERM)
        {
            m_state = READY;
        }
        // 如果协程参与调度器调度，那么应该和调度器的主协程进行swap，而不是线程主协程
        if (m_runInScheduler)
        {
            if (swapcontext(&m_ctx, &(Scheduler::GetMainFiber()->m_ctx)))
            {
                YUAN_ASSERT2(false, "swapcontext");
            }
        }
        else
        {
            if (swapcontext(&m_ctx, &(t_thread_fiber->m_ctx)))
            {
                YUAN_ASSERT2(false, "swapcontext");
            }
        }
    }

    // void Fiber::call()
    // {
    //     //YUAN_LOG_INFO(g_logger) << "in clall()";
    //     SetThis(this);
    //     m_state = EXEC;
    //     if(swapcontext(&t_threadFiber->m_ctx,&m_ctx))
    //     {
    //         YUAN_ASSERT2(false,"swapcontext");
    //     }
    // }

    // void Fiber::back()
    // {
    //     SetThis(t_threadFiber.get());
    //     if(swapcontext(&m_ctx,&t_threadFiber->m_ctx))
    //     {
    //         YUAN_ASSERT2(false,"swapcontext");
    //     }
    // }

    //切换到当前协程执行
    // void Fiber::swapIn()
    // {
    //     SetThis(this);
    //     YUAN_ASSERT1(m_state != EXEC)
    //     m_state = EXEC;
    //     //                  old_ctx,new_ctx -> 主协程，当前协程
    //     if(swapcontext(&Scheduler::GetMainFiber()->m_ctx,&m_ctx))
    //     {
    //         YUAN_ASSERT2(false,"swapcontext");
    //     }
    // }

    //当前协程切换到后台
    // void Fiber::swapOut()
    // {
        // if (t_fiber != Scheduler::GetMainFiber())
        // {
        //     SetThis(Scheduler::GetMainFiber());
        //     if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) // 把子协程的上下文保存在自己的m_ctx中，同时t_threadFiber获得主协程的上下文
        //     {
        //         YUAN_ASSERT2(false, "swapcontext");
        //     }
        // }
        // else
        // {
        //     SetThis(t_threadFiber.get());
        //     if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) // 把子协程的上下文保存在自己的m_ctx中，同时t_threadFiber获得主协程的上下文
        //     {
        //         YUAN_ASSERT2(false, "swapcontext");
        //     }
        // }
    //     SetThis(Scheduler::GetMainFiber());
    //     if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) // 把子协程的上下文保存在自己的m_ctx中，同时t_threadFiber获得主协程的上下文
    //     {
    //         YUAN_ASSERT2(false, "swapcontext");
    //     }
    // }

     //设置当前协程
    void Fiber::SetThis(Fiber* f)
    {
        t_fiber = f;
    }

    //返回当前正在执行的协程，
    Fiber::ptr Fiber::GetThis()
    {
        if(t_fiber)
        {
            return t_fiber->shared_from_this();
        }
        //当前线程还未创建协程，创建第一个协程，该协程为当前线程的主协程
        Fiber::ptr main_fiber(new Fiber);
        YUAN_ASSERT1(t_fiber == main_fiber.get());  //在无参构造函数中就将t_fiber = this
        t_thread_fiber = main_fiber;
        return t_fiber->shared_from_this();
    }

      
    // //协程切换到后台，并设置为Ready状态
    // void Fiber::YieldToReady()
    // {
    //     Fiber::ptr cur = GetThis();
    //     YUAN_ASSERT1(cur->m_state == EXEC);
    //     cur->m_state = READY;
    //     cur->swapOut();
    // }

    // //协程切换到后台，并设置为Hold状态
    // void Fiber::YieldToHold()
    // {
    //     Fiber::ptr cur = GetThis();
    //     YUAN_ASSERT1(cur->m_state == EXEC);
    //     cur->m_state = HOLD;
    //     cur->swapOut();
    // }

    //总的协程数
    uint64_t Fiber::TotalFiber()
    {
        return s_fiber_count;
    }

    //
    void Fiber::MainFunc()
    {
        Fiber::ptr cur = GetThis(); //当前正在运行的协程
        YUAN_ASSERT1(cur);
        try
        {
            cur->m_cb();            //执行传入的函数
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch(const std::exception& e)
        {
            cur->m_state = EXCEPT;
            //std::cout<< "Fiber except" << e.what();
            YUAN_LOG_ERROR(g_logger) << "Fiber except: " << e.what() 
                                    << "fiber id= "<< cur->getId()
                                    << std::endl
                                    << yuan::BacktraceToString();
        }
        catch(...)
        {
            cur->m_state = EXCEPT;
            //std::cout<< "Fiber except";
            YUAN_LOG_ERROR(g_logger) << "Fiber except: "
                                    << "fiber id= "<< cur->getId()
                                    << std::endl
                                    << yuan::BacktraceToString();
        }
        //Fiber::ptr cur = GetThis()获得对象的指针，该对象的引用计数+1,一直不会释放，执行完函数后，它的引用计数不会变成0，也就不会释放
        auto raw_ptr = cur.get();   //得到它的裸指针
        cur.reset();                //释放cur管理的对象，引用计数变为0，对象被销毁
        //raw_ptr->swapOut();
        raw_ptr->yield();

        YUAN_ASSERT2(false,"never reach fiber_id =" + std::to_string(raw_ptr->getId()));//不会运行到这行
    }

    // void Fiber::CallerMainFunc()
    // {
    //     Fiber::ptr cur = GetThis(); //GetThis创建的是主协程，没有回调函数
    //     YUAN_ASSERT1(cur);
    //     try
    //     {
    //         cur->m_cb();
    //         cur->m_cb = nullptr;
    //         cur->m_state = TERM;
    //     }
    //     catch(const std::exception& e)
    //     {
    //         cur->m_state = EXCEPT;
    //         //std::cout<< "Fiber except" << e.what();
    //         YUAN_LOG_ERROR(g_logger) << "Fiber except" << e.what() 
    //                                 << std::endl
    //                                 << yuan::BacktraceToString();
    //     }
    //     catch(...)
    //     {
    //         cur->m_state = EXCEPT;
    //         //std::cout<< "Fiber except";
    //         YUAN_LOG_ERROR(g_logger) << "Fiber except";
    //     }
    //     //Fiber::ptr cur = GetThis()获得对象的指针，该对象的引用计数+1,一直不会释放，执行完函数后，它的引用计数不会变成0，也就不会释放
    //     auto raw_ptr = cur.get();   //得到它的裸指针
    //     cur.reset();                //释放cur管理的对象，引用计数变为0，对象被销毁
    //     raw_ptr->back();

    //     YUAN_ASSERT2(false,"never reach fiber_id =" + std::to_string(raw_ptr->getId()));//不会运行到这行       
    // }
}