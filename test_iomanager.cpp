#include "yuan_iomanager.cpp"
#include "yuan_iomanager.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>


// yuan::Timer::ptr s_timer;
// void test_timer()
// {
//     yuan::IOManager iom(2);
//     s_timer = iom.addTimer(1000,[](){
//         static int i = 0;
//         YUAN_LOG_INFO(g_logger) << "hello timer i=" << i;
        
//         if(++i == 3)
//         {
//             s_timer->reset(2000,true);
//         }
//     },true);
// }
yuan::Logger::ptr g = YUAN_LOG_ROOT();
int sockfd;
void watch_io_read();

void do_io_write()
{
    YUAN_LOG_INFO(g) << "write callback";
    int so_err;
    socklen_t len = size_t(so_err);
    getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&so_err,&len);

    if(so_err)
    {
        YUAN_LOG_INFO(g) << "connect fail";
        return;
    }
    YUAN_LOG_INFO(g) << "connect sucess";
}
void do_io_read()
{
    YUAN_LOG_INFO(g) << "read call back";
    char buf[1024] = {0};
    int readlen = 0;
    readlen = read(sockfd,buf,sizeof(buf));
    if(readlen > 0)
    {
        buf[readlen] = '\0';
        YUAN_LOG_INFO(g) << "read" << readlen << "bytes,read:" << buf;
    }
    else if(readlen == 0)
    {
        YUAN_LOG_INFO(g) << "peer closed";
        close(sockfd);
        return;
    }
    else
    {
        YUAN_LOG_INFO(g) << "error do_io_read";
        close(sockfd);
        return;
    }


    //yuan::IOManager::GetThis()->schedule(watch_io_read);
}

void watch_io_read()
{
    YUAN_LOG_INFO(g) << "watch io_read";
    yuan::IOManager::GetThis()->addEvent(sockfd, yuan::IOManager::READ, do_io_read);
}

void test_io()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    YUAN_ASSERT1(sockfd > 0);

    fcntl(sockfd,F_SETFL,O_NONBLOCK);//将sockfd设置为非阻塞模式

    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(1234);
    inet_pton(AF_INET, "183.2.172.185", &servaddr.sin_addr.s_addr);

    int rt = connect(sockfd, (const sockaddr*)&servaddr, sizeof(servaddr));
    if(rt != 0)
    {
        if(errno == EINPROGRESS)
        {
            YUAN_LOG_INFO(g) << "EINPROGRESS";
            yuan::IOManager::GetThis()->addEvent(sockfd,yuan::IOManager::WRITE,do_io_write);
            yuan::IOManager::GetThis()->addEvent(sockfd,yuan::IOManager::READ,do_io_read);
        }
        else
        {
            YUAN_LOG_INFO(g) << "error";
        }
    }

}

void test_iomanager() 
{
    yuan::IOManager iom;
    iom.schedule(test_io);
}    

int main()
{
    //test_timer();
    test_iomanager();
    return 0;
}