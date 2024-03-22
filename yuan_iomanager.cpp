#include "yuan_iomanager.hpp"
#include "yuan_macro.hpp"
#include "yuan_log.cpp"
#include "yuan_timer.cpp"

#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include<vector>

namespace yuan
{
    static yuan::Logger::ptr g_logger4 = YUAN_LOG_NAME("system");

    IOManager::FdContext::EventContext &IOManager::FdContext::getContext(IOManager::Event event)
    {
        switch (event)
        {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            YUAN_ASSERT2(false, "getContext");
        }
    }
    void IOManager::FdContext::resetContext(EventContext &ctx)
    {
        ctx.scheduler = nullptr;
        ctx.fiber.reset();
        ctx.cb = nullptr;
    }
    void IOManager::FdContext::triggerEvent(Event event)
    {
        YUAN_ASSERT1(events & event);
        events = (Event)(events & ~event);
        EventContext &ctx = getContext(event);
        if (ctx.cb)
        {
            ctx.scheduler->schedule(&ctx.cb);
        }
        else
        {
            ctx.scheduler->schedule(&ctx.fiber);
        }

        //ctx.scheduler = nullptr;
        resetContext(ctx);
        return;
    }

    IOManager::IOManager(size_t threads, bool use_caller, const std::string &name) : Scheduler(threads, use_caller, name)
    {
        // 指定epoll所能处理的最大文件描述符的数量
        m_epfd = epoll_create(5000);
        YUAN_ASSERT1(m_epfd > 0);

        // 创建管道，返回的文件描述符写入在m_tickleFds数组，
        // m_tickleFds[1]表示写入管道的文件描述符，m_tickleFds[0]表示读取管道的文件描述符
        int rt = pipe(m_tickleFds);
        YUAN_ASSERT1(!rt); // 错误

        /*
            EPOLLIN 表示对读取操作感兴趣，即当文件描述符上有数据可读时，将触发事件，
            EPOLLET 表示设置边缘触发模式，这意味着一旦有数据可读，内核将只触发一次事件，直到数据被读取完毕
            如果不设置 EPOLLET 则为水平触发模式，内核会持续触发事件，直到数据被读取完毕。
        */
        epoll_event event;
        memset(&event, 0, sizeof(epoll_event));
        event.events = EPOLLIN | EPOLLET;
        event.data.fd = m_tickleFds[0];

        // m_tickleFds[0]（读管道）设置为非阻塞
        // 在非阻塞模式下，当对文件描述符进行读取或写入操作时，
        // 如果没有立即可用的数据或空间，操作将立即返回，而不是等待数据准备就绪
        rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
        YUAN_ASSERT1(!rt);

        // m_tickleFds[0]添加到epoll实例中进行监听
        rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
        YUAN_ASSERT1(!rt);

        // m_fdContexts.resize(64);
        contextResize(32);

        start();
    }

    IOManager::~IOManager()
    {
        stop();
        close(m_epfd);
        close(m_tickleFds[0]);
        close(m_tickleFds[1]);

        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (m_fdContexts[i])
            {
                delete m_fdContexts[i];
            }
        }
    }

    void IOManager::contextResize(size_t size)
    {
        m_fdContexts.resize(size);
        for (size_t i = 0; i < m_fdContexts.size(); ++i)
        {
            if (!m_fdContexts[i])
            {
                m_fdContexts[i] = new FdContext;
                m_fdContexts[i]->fd = i;
            }
        }
    }

    // 0 成功     -1 错误
    int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
    {
        // 在 m_fdContexts 中找到 fd 对应的 Fdcontext ,没有就分配一个
        FdContext *fd_ctx = nullptr;
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() > fd)
        {
            fd_ctx = m_fdContexts[fd];
            lock.unlock();
        }
        else
        {
            lock.unlock();
            RWMutexType::WriteLock lock2(m_mutex);
            contextResize(fd * 1.5); // 每次容量增加1.5倍
            fd_ctx = m_fdContexts[fd];
        }

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (fd_ctx->events & event) // 添加的事件已经有了
        {
            YUAN_LOG_ERROR(g_logger4) << "addEVent assert fd=" << fd
                                      << "event=" << (EPOLL_EVENTS)event
                                      << "fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->events;
            YUAN_ASSERT1(!(fd_ctx->events & event));
        }

        // 将新的事件加入到 epoll_wait 中
        int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
        epoll_event epevent;
        epevent.events = EPOLLET | fd_ctx->events | event;
        epevent.data.ptr = fd_ctx; // 用 epoll_event 的私有指针保存fd_ctx

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            YUAN_LOG_ERROR(g_logger4) << "epoll_ctl(" << m_epfd << ","
                                      << op << "," << fd << "," << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << "(" << errno << ")(" << strerror(errno) << ") fd_ctx->events="
                                      << (EPOLL_EVENTS)fd_ctx->events;
            return -1;
        }

        ++m_pendingEventCount;

        fd_ctx->events = (Event)(fd_ctx->events | event);
        FdContext::EventCsimlinontext &event_ctx = fd_ctx->getContext(event);
        YUAN_ASSERT1(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);

        event_ctx.scheduler = Scheduler::GetThis();
        if (cb)
        {
            event_ctx.cb.swap(cb);
        }
        else
        {
            event_ctx.fiber = Fiber::GetThis();
            YUAN_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC, "state=" << event_ctx.fiber->getState());
        }
        return 0;
    }

    bool IOManager::delEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(Mutex);
        if (!(fd_ctx->events & event)) // m_fdContexts中没有event这个事件
        {
            return false;
        }

        // 如果清除之后的结果为0，从epoll_wait 中删除该文件描述符
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            YUAN_LOG_ERROR(g_logger4) << "epoll_ctl(" << m_epfd << ","
                                      << op << "," << fd << "," << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << "(" << errno << ")(" << strerror(errno) << ")";
            return false;
        }

        --m_pendingEventCount;
        // 重置fd_ctx
        fd_ctx->events = new_events;
        FdContext::EventContext &event_ctx = fd_ctx->getContext(event);
        fd_ctx->resetContext(event_ctx);
        return true;
    }

    // 找到对应的事件，强制执行
    bool IOManager::cancelEvent(int fd, Event event)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(fd_ctx->mutex);
        if (!(fd_ctx->events & event)) // m_fdContexts中没有event这个事件
        {
            return false;
        }

        // 如果清除之后的结果为0，从epoll_wait 中删除该文件描述符
        Event new_events = (Event)(fd_ctx->events & ~event);
        int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = EPOLLET | new_events;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            YUAN_LOG_ERROR(g_logger4) << "epoll_ctl(" << m_epfd << ","
                                      << op << "," << fd << "," << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << "(" << errno << ")(" << strerror(errno) << ")";
            return false;
        }

        fd_ctx->triggerEvent(event);
        --m_pendingEventCount;
        return true;
    }

    bool IOManager::cancelAll(int fd)
    {
        RWMutexType::ReadLock lock(m_mutex);
        if ((int)m_fdContexts.size() <= fd)
        {
            return false;
        }
        FdContext *fd_ctx = m_fdContexts[fd];
        lock.unlock();

        FdContext::MutexType::Lock lock2(Mutex);
        if (!fd_ctx->events) // m_fdContexts中没有event这个事件
        {
            return false;
        }

        int op = EPOLL_CTL_DEL;
        epoll_event epevent;
        epevent.events = 0;
        epevent.data.ptr = fd_ctx;

        int rt = epoll_ctl(m_epfd, op, fd, &epevent);
        if (rt)
        {
            YUAN_LOG_ERROR(g_logger4) << "epoll_ctl(" << m_epfd << ","
                                      << op << "," << fd << "," << (EPOLL_EVENTS)epevent.events << "):"
                                      << rt << "(" << errno << ")(" << strerror(errno) << ")";
            return false;
        }

        if (fd_ctx->events & READ)
        {
            fd_ctx->triggerEvent(READ);
            --m_pendingEventCount;
        }
        if (fd_ctx->events & WRITE)
        {
            fd_ctx->triggerEvent(WRITE);
            --m_pendingEventCount;
        }

        YUAN_ASSERT1(fd_ctx->events == 0);
        return true;
    }

    IOManager *IOManager::GetThis() // 获取当前的IOManager
    {
        return dynamic_cast<IOManager *>(Scheduler::GetThis());
    }

    //写pipe让idle协程退出，待idle协程yield之后就可以调度其他任务
    void IOManager::tickle()
    {
        if (!hasIdleThreads())
        {
            return;
        }
        // 写入数据
        int rt = write(m_tickleFds[1], "T", 1);
        YUAN_ASSERT1(rt == 1);
    }

    bool IOManager::stopping(uint64_t& timeout)
    {
        timeout = getNextTimer();
        return timeout == ~0ull && m_pendingEventCount == 0 && Scheduler::stopping();
    }

    bool IOManager::stopping()
    { // 剩余要处理的事件为 0
        uint64_t timeout = 0;
        return stopping(timeout);
    }

    // void IOManager::idle()
    // {
    //     YUAN_LOG_DEBUG(g_logger4) << " idle";

    //     // 一次epoll_Wait最多检测256个就绪事件，超过过256会在下一轮处理
    //     const uint64_t MAX_EVENTS = 256;
    //     epoll_event *events = new epoll_event[MAX_EVENTS]();
    //     // 离开idle 会自动释放 events
    //     std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr)
    //                                                { delete[] ptr; });

    //     while (true)
    //     {
    //         if (stopping())
    //         {
    //             YUAN_LOG_INFO(g_logger4) << "name=" << getName() << " idle stopping exit";
    //             break;
    //         }

    //         int rt = 0;
    //         do
    //         {
    //             // 阻塞在epoll_wait ，等待事件发生
    //             static const int MAX_TIMEOUT = 3000;
    //             // 返回就绪的文件描述符个数
    //             rt = epoll_wait(m_epfd, events, MAX_EVENTS, MAX_TIMEOUT);
    //             if (rt < 0 && errno == EINTR)
    //             {
    //             }
    //             else
    //             {
    //                 break;
    //             }
    //         } while (true);

    //         // YUAN_LOG_DEBUG(g_logger) << "epoll_wait(" << m_epfd << ") (rt="
    //         //                         << rt << ") (errno=" << errno << ")(errstr: "
    //         //                         << strerror(errno) << ")";

    //         // 遍历所有发生的事件，根据epoll_event 的私有指针找到对应的fdContext ，进行事件处理
    //         for (int i = 0; i < rt; ++i)
    //         {
    //             epoll_event &event = events[i];

    //             if (event.data.fd == m_tickleFds[0])
    //             {
    //                 uint8_t dummy[256];
    //                 // m_tickleFds[0]用于通知协程调度
    //                 while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
    //                 continue;
    //             }

    //             FdContext *fd_ctx = (FdContext *)event.data.ptr;
    //             FdContext::MutexType::Lock lock(fd_ctx->mutex);
    //             // 错误       套接字对端关闭
    //             if (event.events & (EPOLLERR | EPOLLHUP))
    //             {
    //                 // 同时触发fd的读和写事件
    //                 event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
    //             }

    //             int real_events = NONE;
    //             if (event.events & EPOLLIN)
    //             {
    //                 real_events |= READ;
    //             }
    //             if (event.events & EPOLLOUT)
    //             {
    //                 real_events |= WRITE;
    //             }

    //             if ((fd_ctx->events & real_events) == NONE)
    //             {
    //                 continue;
    //             }

    //             // 剩余的事件
    //             int left_events = (fd_ctx->events * ~real_events);
    //             int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    //             event.events = EPOLLET | left_events;

    //             int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
    //             if (rt2)
    //             {
    //                 YUAN_LOG_ERROR(g_logger4) << "epoll_ctl(" << m_epfd << ","
    //                                           << op << "," << fd_ctx->fd << "," << (EPOLL_EVENTS)event.events << "):"
    //                                           << rt2 << "(" << errno << ")(" << strerror(errno) << ")";
    //                 continue;
    //             }

    //             if (real_events & READ)
    //             {
    //                 fd_ctx->triggerEvent(READ);
    //                 --m_pendingEventCount;
    //             }
    //             if (real_events & WRITE)
    //             {
    //                 fd_ctx->triggerEvent(WRITE);
    //                 --m_pendingEventCount;
    //             }
    //         }

    //         Fiber::ptr cur = Fiber::GetThis();
    //         auto raw_ptr = cur.get();
    //         cur.reset();

    //         raw_ptr->swapOut();
    //     }
    // }


    // git
    void IOManager::idle()
    {
        YUAN_LOG_DEBUG(g_logger4) << "idle";
        const uint64_t MAX_EVNETS = 256;
        epoll_event *events = new epoll_event[MAX_EVNETS]();
        std::shared_ptr<epoll_event> shared_events(events, [](epoll_event *ptr)
                                                   { delete[] ptr; });

        while (true)
        {
            uint64_t next_timeout = 0;
            if ((stopping(next_timeout)))
            {
                YUAN_LOG_INFO(g_logger4) << "name=" << getName()
                                        << " idle stopping exit";
                break;
            }

            int rt = 0;
            do
            {
                static const int MAX_TIMEOUT = 3000;
                if(next_timeout != !0ull)
                {
                    next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
                }
                else
                {
                    next_timeout = MAX_TIMEOUT;
                }
                rt = epoll_wait(m_epfd, events, MAX_EVNETS, int(next_timeout));
                if (rt < 0 && errno == EINTR)
                {
                    continue;
                }
                else
                {
                    break;
                }
            } while (true);


            //收集所有已经超时的定时器，执行回调函数
            std::vector<std::function<void()>> cbs;
            listExpiredCb(cbs);
            if(!cbs.empty())
            {
                //YUAN_LOG_INFO(g_logger) << "on timer cbs.size()=" << cbs.size();
                schedule(cbs.begin(),cbs.end());
                cbs.clear();
            }

            for (int i = 0; i < rt; ++i)
            {
                epoll_event &event = events[i];
                if (event.data.fd == m_tickleFds[0])
                {
                    uint8_t dummy[256];
                    while (read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                    continue;
                }

                FdContext *fd_ctx = (FdContext *)event.data.ptr;
                FdContext::MutexType::Lock lock(fd_ctx->mutex);

                if (event.events & (EPOLLERR | EPOLLHUP))
                {
                    event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->events;
                }
                int real_events = NONE;
                if (event.events & EPOLLIN)
                {
                    real_events |= READ;
                }
                if (event.events & EPOLLOUT)
                {
                    real_events |= WRITE;
                }

                if ((fd_ctx->events & real_events) == NONE)
                {
                    continue;
                }

                int left_events = (fd_ctx->events & ~real_events);
                int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
                event.events = EPOLLET | left_events;

                int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
                if (rt2)
                {
                    YUAN_LOG_ERROR(g_logger4) << "epoll_ctl(" << m_epfd << ", "
                                             << op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                                             << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                    continue;
                }

                // SYLAR_LOG_INFO(g_logger) << " fd=" << fd_ctx->fd << " events=" << fd_ctx->events
                //                          << " real_events=" << real_events;
                
                //处理已经发生的事件
                if (real_events & READ)
                {
                    fd_ctx->triggerEvent(READ);
                    --m_pendingEventCount;
                }
                if (real_events & WRITE)
                {
                    fd_ctx->triggerEvent(WRITE);
                    --m_pendingEventCount;
                }
            }

            Fiber::ptr cur = Fiber::GetThis();
            auto raw_ptr = cur.get();
            cur.reset();

            //raw_ptr->swapOut();
            raw_ptr->yield();
        }
    }

    void IOManager::onTimerInsertedAtFront()
    {
        tickle();
    }
}