#include "yuan_iomanager.cpp"

#include <sys/socket.h>
#include<sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include<fcntl.h>
#include<sys/epoll.h>


yuan::Logger::ptr g_logger = YUAN_LOG_ROOT();
int sock = 0;

void test_Fiber()
{
    YUAN_LOG_INFO(g_logger) << "test_fiber";

    sock = socket(AF_INET,SOCK_STREAM,0);
    fcntl(sock,F_SETFL,O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr,0,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    //addr.sin_addr.s_addr = inet_addr("addr.sin_addr.s_addr");
    inet_pton(AF_INET,"183.2.172.42",&addr.sin_addr.s_addr);
    
    if(!connect(sock,(const sockaddr*)&addr,sizeof(addr)))
    {

    }
    else if(errno == EINPROGRESS)
    {
        YUAN_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
        yuan::IOManager::GetThis()->addEvent(sock,yuan::IOManager::READ,[]()
        {
            YUAN_LOG_INFO(g_logger) << "read callback" ;
        });

        // yuan::IOManager::GetThis()->addEvent(sock,yuan::IOManager::WRITE,[]()
        // {
        //     YUAN_LOG_INFO(g_logger) << "write callback" ;
        //     yuan::IOManager::GetThis()->cancelEvent(sock,yuan::IOManager::READ);
        //     close(sock);
        // });
    }
    else
    {
        YUAN_LOG_INFO(g_logger) << "else" << errno << " " << strerror(errno);
    }
}

void test1()
{
    yuan::IOManager iom(2,false);
    iom.schedule(&test_Fiber);
}


int main()
{
    test1();

    return 0;
}