#ifndef __YUAN_FIBER_HPP_
#define __YUAN_FIBER_HPP_

#include<ucontext.h>
#include<memory>
#include<functional>
#include "yuan_thread.hpp"

namespace yuan
{
    class Fiber:public std::enable_shared_from_this<Fiber>
    {
    public:
       typedef std::shared_ptr<Fiber> ptr;
        enum State
        {
            INIT,
            HOLD,
            EXEC,
            TERM,
            READY,
            EXCEPT
        };
    private:
        Fiber();
    public:
        Fiber(std::function<void()> cb,size_t stacksize = 0);
        ~Fiber();

        //重置协程状态和入口函数,能重置的协程状态只能为（INIT，TERM）
        void reset(std::function<void()> cb);  
        //切换到当前协程执行
        void swapIn();
        //当前协程切换到后台
        void swapOut();

        uint64_t getId() const {return m_id;}
    public:
        //设置当前协程
        static void SetThis(Fiber* f);

        //返回当前协程
        static Fiber::ptr GetThis();
        

        //协程切换到后台，并设置为Ready状态
        static void YieldToReady();

        //协程切换到后台，并设置为Hold状态
        static void YieldToHold();

        //总的协程数
        static uint64_t TotalFiber();

        //
        static void MainFunc();

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