#ifndef __YUAN_IOMANAGER_HPP__
#define __YUAN_IOMANAGER_HPP__
#include "yuan_scheduler.cpp"
#include "yuan_timer.hpp"

namespace yuan
{

class IOManager:public Scheduler,public TimerManager
{
public:
    typedef std::shared_ptr<IOManager> ptr;
    typedef RWMutex RWMutexType;

    enum Event
    {
        NONE = 0x0,
        READ = 0x1,     //EPOLLIIN
        WRITE = 0x4,    //EPOLLOUT
    };

private:
    struct FdContext
    {
        typedef Mutex MutexType;
        struct EventContext
        {
            Scheduler* scheduler = nullptr;             //事件执行的scheduler
            Fiber::ptr fiber;                           //事件协程
            std::function<void()> cb;                   //事件的回调函数
        };

        EventContext& getContext(Event event);          //获取事件的上下文
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event); 
        
        
        EventContext read;          //读事件           
        EventContext write;
        int fd = 0;
        Event events = NONE;      //已经注册的事件
        MutexType mutex;
    };
public:
    IOManager(size_t threads = 1,bool use_caller = true,const std::string& name = "IOManager");
    ~IOManager();

    //0 成功   -1 错误
    int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
    bool delEvent(int fd,Event event);
    bool cancelEvent(int fd,Event event);

    bool cancelAll(int fd);

    static IOManager* GetThis();//获取当前的IOManager

protected:
    void tickle() override;
    bool stopping() override;
    void idle() override;
    void onTimerInsertedAtFront() override;
    
    void contextResize(size_t size);
    bool stopping(uint64_t& timeout);
private:
    int m_epfd;
    int m_tickleFds[2];

    std::atomic<int> m_pendingEventCount = {0};//现在要等待执行事件的数量
    RWMutexType m_mutex;
    std::vector<FdContext*> m_fdContexts;

};





}

#endif