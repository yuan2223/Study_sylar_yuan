#include "yuan_fiber.hpp"
#include "yuan_config.hpp"
#include "yuan_macro.hpp"
#include<atomic>

namespace yuan
{
    static Logger::ptr g_logger = YUAN_LOG_NAME("system");
    static std::atomic<uint64_t> s_fiber_id {0};
    static std::atomic<uint64_t> s_fiber_count {0};

    static thread_local Fiber* t_fiber = nullptr;   //线程里面返回当前协程,主协程
    static thread_local Fiber::ptr t_threadFiber = nullptr;
    
    static ConfigVar<uint32_t>::ptr g_fiber_stack_size = Config::Lookup<uint32_t>("fiber.stack_size",1024*1024,"fiber stack size");
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
        m_state = EXEC;
        SetThis(this);

        if(getcontext(&m_ctx))
        {
            YUAN_ASSERT2(false,"getcontext");
        }
        ++s_fiber_count;

        YUAN_LOG_DEBUG(g_logger) << "Fiber::Fiber() function";
    }
    //每个子协程都有独立的占空间
    Fiber::Fiber(std::function<void()> cb,size_t stacksize):m_id(++s_fiber_id),m_cb(cb)
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

        makecontext(&m_ctx,&Fiber::MainFunc,0);
        YUAN_LOG_DEBUG(g_logger) << "Fiber::Fiber id=" << m_id;
    }
    Fiber::~Fiber()
    {
        --s_fiber_count;
        if(m_stack)
        {
            YUAN_ASSERT1(m_state == TERM || m_state == EXCEPT || m_state == INIT);
            
            StackAllocator::Dealloc(m_stack,m_stacksize);
        }
        else
        {
            //没有栈,说明是主协程
            YUAN_ASSERT1(!m_cb);
            YUAN_ASSERT1(m_state == EXEC);//

            Fiber* cur = t_fiber;
            if(cur == this)
            {
                SetThis(nullptr);
            }
        }
        YUAN_LOG_DEBUG(g_logger) << "Fiber::~Fiber() id=" << m_id;
    }
   

    //重置协程状态和入口函数,能重置的协程状态只能为（INIT，TERM）,主要是充分利用内存，一个协程运行完，但内存没有释放，基于这块内存继续初始化
    void Fiber::reset(std::function<void()> cb)
    {
        YUAN_ASSERT1(m_stack);
        YUAN_ASSERT1(m_state == TERM || m_state == EXCEPT || m_state == INIT);

        m_cb = cb;
        if(getcontext(&m_ctx))
        {
            YUAN_ASSERT2(false,"getcontext");
        }
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;
        m_state = INIT;
    }

    //切换到当前协程执行
    void Fiber::swapIn()
    {
        SetThis(this);
        YUAN_ASSERT1(m_state != EXEC)
        m_state = EXEC;
        //                  old_ctx,new_ctx -> 主协程，当前协程
        if(swapcontext(&t_threadFiber->m_ctx,&m_ctx))
        {
            YUAN_ASSERT2(false,"swapcontext");
        }
    }

    //当前协程切换到后台
    void Fiber::swapOut()
    {
        SetThis(t_threadFiber.get());
        if(swapcontext(&m_ctx,&t_threadFiber->m_ctx))//把子协程的上下文保存在自己的m_ctx中，同时t_threadFiber获得主协程的上下文
        {
             YUAN_ASSERT2(false,"swapcontext");
        }
    }

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
        t_threadFiber = main_fiber;
        return t_fiber->shared_from_this();
    }

      
    //协程切换到后台，并设置为Ready状态
    void Fiber::YieldToReady()
    {
        Fiber::ptr cur = GetThis();
        YUAN_ASSERT1(cur->m_state == EXEC);
        cur->m_state = READY;
        cur->swapOut();
    }

    //协程切换到后台，并设置为Hold状态
    void Fiber::YieldToHold()
    {
        Fiber::ptr cur = GetThis();
        YUAN_ASSERT1(cur->m_state == EXEC);
        cur->m_state = HOLD;
        cur->swapOut();
    }

    //总的协程数
    uint64_t Fiber::TotalFiber()
    {
        return s_fiber_count;
    }

    //
    void Fiber::MainFunc()
    {
        Fiber::ptr cur = GetThis(); //GetThis创建的是主协程，没有回调函数
        YUAN_ASSERT1(cur);
        try
        {
            cur->m_cb();
            cur->m_cb = nullptr;
            cur->m_state = TERM;
        }
        catch(const std::exception& e)
        {
            cur->m_state = EXCEPT;
            //std::cout<< "Fiber except" << e.what();
            YUAN_LOG_ERROR(g_logger) << "Fiber except" << e.what();
        }
        catch(...)
        {
            cur->m_state = EXCEPT;
            //std::cout<< "Fiber except";
            YUAN_LOG_ERROR(g_logger) << "Fiber except";
        }
        //Fiber::ptr cur = GetThis()获得对象的指针，该对象的引用计数+1,一直不会释放，执行完函数后，它的引用计数不会变成0，也就不会释放
        auto raw_ptr = cur.get();   //得到它的裸指针
        cur.reset();                //释放cur管理的对象，引用计数变为0，对象被销毁
        raw_ptr->swapOut();

        YUAN_ASSERT2(false,"never reach");//不会运行到这行
    }
}