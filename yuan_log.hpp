/*
		日志模块
		工作流程
		1、初始化LogFormatter，LogAppender, Logger
		2、通过宏定义提供流式风格和格式化风格的日志接口。每次写日志时，通过宏自动生成对应的日志事件LogEvent，并且将日志事件和日志器Logger包装到一起，生成一个LogEventWrap对象。
		3、日志接口执行结束后，LogEventWrap对象析构，在析构函数里调用Logger的log方法将日志事件进行输出。
* */

#ifndef __YUAN_LOG_HPP__
#define __YUAN_LOG_HPP__

#include <iostream>
#include <string>
#include <stdint.h>
#include<stdarg.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <functional>
#include "yuan_until.cpp"
#include "yuan_singleton.hpp"



#define YUAN_LOG_LEVEL(logger,level) \
	if(logger->getLevel() <= level) \
		yuan::LogEventWrap(yuan::LogEvent::ptr(new yuan::LogEvent(logger,level,\
							__FILE__,__LINE__,0,yuan::GetThreadId(),\
							yuan::GetFiberId(),time(0)))).getSS()

#define YUAN_LOG_DEBUG(logger) YUAN_LOG_LEVEL(logger,yuan::LogLevel::DEBUG)
#define YUAN_LOG_INFO(logger) YUAN_LOG_LEVEL(logger,yuan::LogLevel::INFO)
#define YUAN_LOG_WARN(logger) YUAN_LOG_LEVEL(logger,yuan::LogLevel::WARN)
#define YUAN_LOG_ERROR(logger) YUAN_LOG_LEVEL(logger,yuan::LogLevel::ERROR)
#define YUAN_LOG_FATAL(logger) YUAN_LOG_LEVEL(logger,yuan::LogLevel::FATAL)


#define YUAN_LOG_FMT_LEVEL(logger, level, fmt, ...) \
    if(logger->getLevel() <= level) \
        yuan::LogEventWrap(yuan::LogEvent::ptr(new yuan::LogEvent(logger, level, \
                        __FILE__, __LINE__, 0, yuan::GetThreadId(),\
                yuan::GetFiberId(), time(0)))).getEvent()->format(fmt, __VA_ARGS__)


#define YUAN_LOG_FMT_DEBUG(logger,fmt, ...) YUAN_LOG_FMT_LEVEL(logger,yuan::LogLevel::Level::DEBUG,fmt,__VA_ARGS__)
#define YUAN_LOG_FMT_INFO(logger,fmt, ...) YUAN_LOG_FMT_LEVEL(logger,yuan::LogLevel::Level::INFO,fmt,__VA_ARGS__)
#define YUAN_LOG_FMT_WARN(logger,fmt, ...) YUAN_LOG_FMT_LEVEL(logger,yuan::LogLevel::Level::WARN,fmt,__VA_ARGS__)
#define YUAN_LOG_FMT_ERROR(logger,fmt, ...) YUAN_LOG_FMT_LEVEL(logger,yuan::LogLevel::Level::ERROR,fmt,__VA_ARGS__)
#define YUAN_LOG_FMT_FATAL(logger,fmt, ...) YUAN_LOG_FMT_LEVEL(logger,yuan::LogLevel::Level::FATAL,fmt,__VA_ARGS__)

#define YUAN_LOG_ROOT() yuan::LoggerMgr::GetInstance()->getRoot()
#define YUAN_LOG_NAME(name) yuan::LoggerMgr::GetInstance()->getLogger(name)

namespace yuan
{
	class Logger;
	class LogManager;

	// 日志级别
	class LogLevel
	{
	public:
		enum Level
		{
			UNKONW = 0,
			DEBUG = 1,
			INFO = 2,
			WARN = 3,
			ERROR = 4,
			FATAL = 5

		};
		static const char *ToString(LogLevel::Level level);
		static LogLevel::Level FromString(const std::string& str);
	};

	class LogEvent
	{
		/*
		日志事件，用于记录日志现场，比如该日志的级别，文件名/行号，日志消息，线程/协程号，所属日志器名称等。
		*/
	public:
		typedef std::shared_ptr<LogEvent> ptr;
		LogEvent(std::shared_ptr<Logger> ptr, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse,
				 uint32_t thread_id, uint32_t fiber_id, uint64_t time);

		const char *getFile() const { return m_file; }
		uint32_t get_Elapse() const { return m_line; }
		int32_t getLine() const { return m_line; }
		uint32_t get_ThreadId() const { return m_threadId; }
		uint32_t get_FiberId() const { return m_fiberId; }
		uint64_t getTime() const { return m_time; }
		std::string getContent() const { return m_ss.str(); }
		std::stringstream &getSS() { return m_ss; }
		std::shared_ptr<Logger> getLogger() const { return m_logger; }
		LogLevel::Level getLevel() const { return m_level; }

		void format(const char *fmt, ...);
		void format(const char *fmt, va_list al);

	private:
		const char *m_file = nullptr; // 文件名
		int32_t m_line = 0;			  // 行号
		uint32_t m_elapse = 0;		  // 程序启动到现在的时间，单位是毫秒
		uint32_t m_threadId = 0;	  // 线程id
		uint32_t m_fiberId = 0;		  // 协程ido
		uint64_t m_time = 0;		  // 时间戳
		std::stringstream m_ss;

		std::shared_ptr<Logger> m_logger;
		LogLevel::Level m_level;
	};

	class LogEventWrap
	{
		/*
		日志事件包装类，其实就是将日志事件和日志器包装到一起，因为一条日志只会在一个日志器上进行输出。将日志事件和日志器包装到一起后，方便通过宏定义来简化日志模块的使用。
		另外，LogEventWrap还负责在构建时指定日志事件和日志器，在析构时调用日志器的log方法将日志事件进行输出。
		*/
	public:
		LogEventWrap(LogEvent::ptr e);
		~LogEventWrap();
		LogEvent::ptr getEvent() const { return m_event; }
		std::stringstream &getSS();

	private:
		LogEvent::ptr m_event;
	};

	class LogFormatter
	{
		/*
		日志格式器，用于格式化一个日志事件。
		该类构建时可以指定pattern，表示如何进行格式化。提供format方法，用于将日志事件格式化成字符串。
		*/
	public:
		typedef std::shared_ptr<LogFormatter> ptr;
		LogFormatter(const std::string &pattern);

		std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
		std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

	public:
		class FormatItem
		{
		public:
			typedef std::shared_ptr<FormatItem> ptr;
			FormatItem(const std::string &fmt = "") {}
			virtual ~FormatItem(){};
			virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
		};

		// 日志格式的解析
		void init();

		bool isError() const {return m_error;}
		const std::string getPattern() const {return m_pattern;}

	private:
		std::string m_pattern;
		std::vector<FormatItem::ptr> m_items;
		bool m_error = false;
	};

	class LogAppender
	{
		/*
		日志输出器，用于将一个日志事件输出到对应的输出地。该类内部包含一个LogFormatter成员和一个log方法，日志事件先经过
		LogFormatter格式化后再输出到对应的输出地。从这个类可以派生出不同的Appender类型，比如StdoutLogAppender和FileLogAppender，分别表示输出到终端和文件。
		*/
	friend class Logger;

	public:
		typedef std::shared_ptr<LogAppender> ptr;
		virtual ~LogAppender() {}

		virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
		virtual std::string toYamlString() = 0;

		void setFormatter(LogFormatter::ptr val)
		{
			m_formatter = val;
			if (m_formatter)
			{
				m_hasFormatter = true;
			}
			else
			{
				m_hasFormatter = false;
			}
		}

		LogFormatter::ptr getFormatter() const { return m_formatter; }

		LogLevel::Level getLevel() const { return m_level; }
		void setLevel(LogLevel::Level val) { m_level = val; }

	protected:
		LogLevel::Level m_level;
		bool m_hasFormatter = false;
		LogFormatter::ptr m_formatter;
	};

	class Logger : public std::enable_shared_from_this<Logger> // 把自己作为指针传入函数
	{
		/*
		日志器，负责进行日志输出。一个Logger包含多个LogAppender和一个日志级别，提供log方法，传入日志事件，
		判断该日志事件的级别高于日志器本身的级别之后调用LogAppender将日志进行输出，否则该日志被抛弃。
		*/
		friend class LogManager;
	public:
		typedef std::shared_ptr<Logger> ptr;
		Logger(const std::string &name = "root");

		void log(LogLevel::Level level, LogEvent::ptr event);

		void debug(LogEvent::ptr event);
		void info(LogEvent::ptr event);
		void warn(LogEvent::ptr event);
		void error(LogEvent::ptr event);
		void fatal(LogEvent::ptr event);

		void addAppender(LogAppender::ptr appender);
		void delAppender(LogAppender::ptr appender);
		void clearAppender();

		LogLevel::Level getLevel() const
		{
			return m_level;
		}
		// 设置日志级别
		void setLevel(LogLevel::Level val)
		{
			m_level = val;
		}

		const std::string &getName() const { return m_name; }

		void setFormatter(LogFormatter::ptr val);
		void setFormatter(const std::string& val);
		LogFormatter::ptr getFormatter();

		std::string toYamlString();

	private:
		std::string m_name;						 // 日志名称
		LogLevel::Level m_level;				 // 日志级别
		std::list<LogAppender::ptr> m_appenders; // Appender集合
		LogFormatter::ptr m_formatter;
		Logger::ptr m_root;
	};

	// 输出到控制台的Appender
	class StdoutLogAppender : public LogAppender
	{
	public:
		typedef std::shared_ptr<StdoutLogAppender> ptr;
		void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;
		std::string toYamlString() override;
	};

	// 输出到文件的Appender
	class FileLogAppender : public LogAppender
	{
	public:
		typedef std::shared_ptr<FileLogAppender> ptr;
		FileLogAppender(const std::string &filename);
		void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override;

		std::string toYamlString() override;
		// 重新打开文件，打开成功返回true
		bool reopen();

	private:
		std::string m_filename; // 输出到文件的文件名
		std::ofstream m_filestream;
		uint64_t m_lastTime = 0;
	};

	class LogManager
	{
		/*
		日志器管理类，单例模式，用于统一管理所有的日志器，提供日志器的创建与获取方法。LogManager自带一个root Logger，用于为日志模块提供一个初始可用的日志器。
		*/
	public:
		LogManager();
		Logger::ptr getLogger(const std::string &name);
		void init();
		Logger::ptr getRoot() const { return m_root; }

		std::string toYamlString();

	private:
		std::map<std::string, Logger::ptr> m_loggers;
		Logger::ptr m_root;
	};

	typedef yuan::Singleton<LogManager> LoggerMgr;

}
#endif // !__YUAN_LOG_H__

