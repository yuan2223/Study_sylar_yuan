#include<iostream>
#include "yuan_log.cpp"




int main()
{
    //g++ test_log.cpp -lyaml-cpp -pthread -o test_log
    yuan::Logger::ptr logger(new yuan::Logger);
    logger->addAppender(yuan::LogAppender::ptr(new yuan::StdoutLogAppender));

    yuan::FileLogAppender::ptr file_appender(new yuan::FileLogAppender("study_sylar/log.txt"));
    yuan::LogFormatter::ptr fmt(new yuan::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(yuan::LogLevel::ERROR);
    logger->addAppender(file_appender);

   

    

    // yuan::LogEvent::ptr event(new yuan::LogEvent(__FILE__,__LINE__,0,yuan::GetThreadId(),yuan::GetFiberId(),time(0)));
    // logger->log(yuan::LogLevel::DEBUG,event);

    std::cout<<"hello world"<<std::endl;

    YUAN_LOG_INFO(logger)<<"test info";
    YUAN_LOG_ERROR(logger)<<"test error";

    YUAN_LOG_FMT_ERROR(logger,"test fmt error","aa");
    auto l = yuan::LoggerMgr::GetInstance()->getLogger("xx");
    YUAN_LOG_INFO(l)<<"xxx";

    return 0;

}
