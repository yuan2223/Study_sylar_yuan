#ifndef __YUAN_FIBER_HPP_
#define __YUAN_FIBER_HPP_

#include<ucontext.h>
#include<memory>
#include<functional>
#include "yuan_thread.hpp"

namespace yuan
{
    class Scheduler;
    class Fiber:public std::enable_shared_from_this<Fiber>
    {
    friend Scheduler;
    public:
       typedef std::shared_ptr<Fiber> ptr;
        enum State
        {
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY,
            EXCEPT          //结束
        };
    private:
        Fiber();
    public:
        Fiber(std::function<void()> cb,size_t stacksize = 0,bool use_caller = false);
        ~Fiber();

        //重置协程状态和入口函数,能重置的协程状态只能为（INIT，TERM）
        void reset(std::function<void()> cb);  
        //切换到当前协程执行
        void swapIn();

        //当前协程切换到后台
        void swapOut();

        //将当前线程切换为执行状态，执行的为当前线程的主协程
        void call();

        //将当前线程切换到后台，执行为该协程，返回到线程的主协程
        void back();

        uint64_t getId() const {return m_id;}

        //返回当前状态
        State getState() const {return m_state;}
    public:
        //设置当前协程
        static void SetThis(Fiber* f);

        //返回当前正在执行的协程
        static Fiber::ptr GetThis();
        

        //协程切换到后台，并设置为Ready状态
        static void YieldToReady();

        //协程切换到后台，并设置为Hold状态
        static void YieldToHold();

        //总的协程数
        static uint64_t TotalFiber();

        //
        static void MainFunc();
        static void CallerMainFunc();

        static uint64_t GetFiberId();
        

    private:
        uint64_t m_id = 0;
        uint32_t m_stacksize = 0;
        State m_state = INIT;

        ucontext_t m_ctx;
        void* m_stack = nullptr;
        std::function<void()> m_cb;
    };
}


#endif