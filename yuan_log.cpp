#include "yuan_log.hpp"
#include "yuan_config.hpp"

namespace yuan
{
	const char *LogLevel::ToString(LogLevel::Level level)
	{
		/*switch (level)
		{
		case sylar::LogLevel::UNKONW:
			break;
		case sylar::LogLevel::DEBUG:
			break;
		case sylar::LogLevel::INFO:
			break;
		case sylar::LogLevel::WARN:
			break;
		case sylar::LogLevel::ERROR:
			break;
		case sylar::LogLevel::FATAL:
			break;
		default:
			break;
		}*/
		switch (level)
		{
#define XX(name)         \
	case LogLevel::name: \
		return #name;    \
		break;

			XX(DEBUG);
			XX(INFO);
			XX(WARN);
			XX(ERROR);
			XX(FATAL);
#undef XX
		default:
			return "UNKNOW";
		}
		return "UNKNOW";
	}

	LogLevel::Level LogLevel::FromString(const std::string& str)
	{
#define XX(name)\
	if(str == #name)\
	{\
		return LogLevel::name;\
	}
	XX(DEBUG);
	XX(INFO);
	XX(WARN);
	XX(ERROR);
	XX(FATAL);
	return LogLevel::UNKONW;
#undef XX
	}


	class MessageFormatItem : public LogFormatter::FormatItem
	{
	public:
		MessageFormatItem(const std::string &str = "") {}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << event->getContent();
		}
	};
	class LevelFormatItem : public LogFormatter::FormatItem
	{
	public:
		LevelFormatItem(const std::string &str = "") {}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << LogLevel::ToString(level);
		}
	};
	class ElapseFormatItem : public LogFormatter::FormatItem
	{
	public:
		ElapseFormatItem(const std::string &str = "") {}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << event->get_Elapse();
		}
	};
	class NameFormatItem : public LogFormatter::FormatItem
	{
	public:
		NameFormatItem(const std::string &str = "") {}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << event->getLogger()->getName();
		}
	};
	class ThreadIdFormatItem : public LogFormatter::FormatItem
	{
	public:
		ThreadIdFormatItem(const std::string &str = "") {}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << event->get_ThreadId();
		}
	};
	class FiberIdFormatItem : public LogFormatter::FormatItem
	{
	public:
		FiberIdFormatItem(const std::string &str = "") {}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << event->get_FiberId();
		}
	};
	class DateTimeFormatItem : public LogFormatter::FormatItem
	{
	public:
		DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S") : m_format(format)
		{
			if (m_format.empty())
			{
				m_format = "%Y-%m-%d %H:%M:%S";
			}
		}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			struct tm tm;
			time_t time = event->getTime();
			// linux调用 localtime_r(&time,&tm);
			localtime_r(&time, &tm);
			char buf[64];
			strftime(buf, sizeof(buf), m_format.c_str(), &tm);
			os << buf;

			os << event->getTime();
		}

	private:
		std::string m_format;
	};
	class FilenameFormatItem : public LogFormatter::FormatItem
	{
	public:
		FilenameFormatItem(const std::string &str = "") {}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << event->getFile();
		}
	};
	class NewLineFormatItem : public LogFormatter::FormatItem
	{
	public:
		NewLineFormatItem(const std::string &str = "") {}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << std::endl;
		}
	};
	class LineFormatItem : public LogFormatter::FormatItem
	{
	public:
		LineFormatItem(const std::string &str = "") {}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << event->getLine();
		}
	};
	class StringFormatItem : public LogFormatter::FormatItem
	{
	public:
		StringFormatItem(const std::string &str) : FormatItem(str), m_string(str) {}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << m_string;
		}

	private:
		std::string m_string;
	};
	class TabFormatItem : public LogFormatter::FormatItem
	{
	public:
		TabFormatItem(const std::string &str = "") {}
		void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override
		{
			os << "\t";
		}

	private:
		std::string m_string;
	};

	/*********************************************** LogEvent *****************************************************************/
	LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse,
					   uint32_t thread_id, uint32_t fiber_id, uint64_t time)
		: m_logger(logger), m_level(level), m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id),
		  m_fiberId(fiber_id), m_time(time)
	{
	}

	void LogEvent::format(const char *fmt, ...)
	{
		va_list al;
		va_start(al, fmt);
		va_end(al);
	}
	void LogEvent::format(const char *fmt, va_list al)
	{
		char *buf = nullptr;
		int len = vasprintf(&buf, fmt, al);
		if (len != -1)
		{
			m_ss << std::string(buf, len);
			free(buf);
		}
	}

	LogEventWrap::LogEventWrap(LogEvent::ptr e) : m_event(e)
	{
	}
	LogEventWrap::~LogEventWrap()
	{
		m_event->getLogger()->log(m_event->getLevel(), m_event);
	}
	std::stringstream &LogEventWrap::getSS()
	{
		return m_event->getSS();
	}
	Logger::Logger(const std::string &name) : m_name(name), m_level(LogLevel::DEBUG)
	{
		m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
		// m_formatter.reset(new LogFormatter("[%d]%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
	}

	void Logger::setFormatter(LogFormatter::ptr val)
	{
		m_formatter = val;
	}
	void Logger::setFormatter(const std::string& val)
	{
		yuan::LogFormatter::ptr new_val(new yuan::LogFormatter(val));
		if(new_val->isError())
		{
			std::cout << "Logger setFormatter name= " << m_name 
						<< " value= " << val << " invalid formatter "
						<< std::endl;
			return;
		}
		m_formatter = new_val;
	}
	LogFormatter::ptr Logger::getFormatter()
	{
		return m_formatter;
	}


	void Logger::log(LogLevel::Level level, LogEvent::ptr event)
	{
		if (level >= m_level)
		{
			auto self = shared_from_this();
			if(!m_appenders.empty())
			{
				for (const auto &i : m_appenders)
				{
					i->log(self, level, event);
				}
			}
			else if(m_root)
			{
				m_root->log(level,event);
			}
			
		}
	}

	void Logger::debug(LogEvent::ptr event)
	{
		log(LogLevel::DEBUG, event);
	}
	void Logger::info(LogEvent::ptr event)
	{
		log(LogLevel::INFO, event);
	}
	void Logger::warn(LogEvent::ptr event)
	{
		log(LogLevel::WARN, event);
	}
	void Logger::error(LogEvent::ptr event)
	{
		log(LogLevel::ERROR, event);
	}

	void Logger::fatal(LogEvent::ptr event)
	{
		log(LogLevel::FATAL, event);
	}

	void Logger::addAppender(LogAppender::ptr appender)
	{
		if (!appender->getFormatter())
		{
			appender->setFormatter(m_formatter);
		}
		m_appenders.push_back(appender);
	}
	void Logger::delAppender(LogAppender::ptr appender)
	{
		for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
		{
			if (*it == appender)
			{
				m_appenders.erase(it);
				break;
			}
		}
	}
	void Logger::clearAppender()
	{
		m_appenders.clear();
	}

	FileLogAppender::FileLogAppender(const std::string &filename) : m_filename(filename)
	{
	}
	void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
	{
		if (level >= m_level)
		{
			m_filestream << m_formatter->format(logger, level, event);
		}
	}
	bool FileLogAppender::reopen()
	{
		if (m_filestream)
		{
			m_filestream.close();
		}
		m_filestream.open(m_filename);
		return !!m_filestream;
	}
	void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
	{
		if (level >= m_level)
		{
			std::cout << m_formatter->format(logger, level, event);
		}
	}
	LogFormatter::LogFormatter(const std::string &pattern) : m_pattern(pattern)
	{
		init();
	}
	std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
	{
		std::stringstream ss;
		for (auto &i : m_items)
		{
			i->format(ss, logger, level, event); // 是FormatItem中的format函数
		}
		return ss.str();
	}
	// 	void LogFormatter::init()
	// 	{
	// 		//str	format	type
	// 		std::vector<std::tuple<std::string, std::string, int>> vec;
	// 		std::string nstr;
	// 		for (size_t i = 0; i < m_pattern.size(); ++i)
	// 		{
	// 			if (m_pattern[i] != '%')
	// 			{
	// 				nstr.append(1, m_pattern[i]);
	// 				continue;
	// 			}

	// 			//两个%连在一起
	// 			if ((i + 1) < m_pattern.size())
	// 			{
	//				if(m_pattern[i] != '%')
	//				{
	//					nstr.append(1,'%');
	//					continue;
	//				}
	// 			}

	// 			size_t n = i + 1;
	// 			int fmt_status = 0;
	// 			size_t fmt_begin = 0;

	// 			std::string str;
	// 			std::string fmt;
	// 			while (n < m_pattern.size())
	// 			{
	// 				if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}'))//接收字母
	// 				{
	//					str = m_pattern[n].substr(i+1,n-i-1);
	// 					break;
	// 				}
	// 				if (fmt_status == 0)
	// 				{
	// 					if (m_pattern[n] == '{')
	// 					{
	// 						str = m_pattern.substr(i + 1, n - i - 1);
	// 						fmt_status = 1;//解析格式
	// 						fmt_begin = n;
	// 						++n;
	// 						continue;
	// 					}
	// 				}
	// 				//开始解析格式
	// 				if (fmt_status == 1)
	// 				{
	// 					if (m_pattern[n] == '}')
	// 					{
	// 						fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
	// 						fmt_status = 0;
	//						++n;
	// 						break;
	// 					}
	// 				}
	// 				++n;
	// 			}
	// 			if (fmt_status == 0)
	// 			{
	// 				if (!nstr.empty())
	// 				{
	// 					vec.push_back(std::make_tuple(nstr, "", 0));
	// 					nstr.clear();
	// 				}
	// 				str = m_pattern.substr(i + 1, n - i - 1);
	// 				vec.push_back(std::make_tuple(str, fmt, 1));
	// 				i = n - 1;
	// 			}
	// 			else if (fmt_status == 1)
	// 			{
	// 				std::cout << "pattern parse error: " << m_pattern << "-" << m_pattern.substr(i) << std::endl;
	// 				vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 1));
	// 			}
	// 			else if (fmt_status == 2)
	// 			{
	// 				if (!nstr.empty())
	// 				{
	// 					vec.push_back(std::make_tuple(nstr, "", 0));
	// 					nstr.clear();
	// 				}
	// 				vec.push_back(std::make_tuple(str, fmt, 1));
	// 				i = n - 1;
	// 			}
	// 		}
	// 		if (!nstr.empty())
	// 		{
	// 			vec.push_back(std::make_tuple(nstr, "", 0));
	// 		}
	// 		static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
	// #define XX(str,C) \
// 	{#str,[](const std::string& fmt){return FormatItem::ptr(new C(fmt));}}

	// 	XX(m,MessageFormatItem),
	// 	XX(p,LevelFormatItem),
	// 	XX(r, ElapseFormatItem),
	// 	XX(c, NameFormatItem),
	// 	XX(t, ThreadIdFormatItem),
	// 	XX(n, NewLineFormatItem),
	// 	XX(d, DateTimeFormatItem),
	// 	XX(f, FilenameFormatItem),
	// 	XX(l, LineFormatItem),
	// 	XX(T, TabFormatItem),
	// 	XX(F, FiberIdFormatItem)
	// #undef XX
	// 		};
	// 			/*
	// 			%m -- 消息体
	// 			%p -- level
	// 			%r -- 启动后的时间
	// 			%c -- 日志名称
	// 			%t -- 线程id
	// 			%n -- 回车换行
	// 			%d -- 时间
	// 			%f -- 文件名
	// 			%l -- 行号
	// 		*/

	// 		for (auto& i : vec)
	// 		{
	// 			if (std::get<2>(i) == 0)
	// 			{
	// 				m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
	// 			}
	// 			else
	// 			{
	// 				auto it = s_format_items.find(std::get<0>(i));
	// 				if (it == s_format_items.end())
	// 				{
	// 					m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
	// 				}
	// 				else
	// 				{
	// 					m_items.push_back(it->second(std::get<1>(i)));
	// 				}
	// 			}
	// 			//std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << " )- (" << std::get<2>(i) << ")" << std::endl;
	// 		}
	// 	}

	void LogFormatter::init()
	{
		// str, format, type
		std::vector<std::tuple<std::string, std::string, int>> vec;
		std::string nstr;
		for (size_t i = 0; i < m_pattern.size(); ++i)
		{
			if (m_pattern[i] != '%')
			{
				nstr.append(1, m_pattern[i]);
				continue;
			}

			if ((i + 1) < m_pattern.size())
			{
				if (m_pattern[i + 1] == '%')
				{
					nstr.append(1, '%');
					continue;
				}
			}

			size_t n = i + 1;
			int fmt_status = 0;
			size_t fmt_begin = 0;

			std::string str;
			std::string fmt;
			while (n < m_pattern.size())
			{
				if (!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}'))
				{
					str = m_pattern.substr(i + 1, n - i - 1);
					break;
				}
				if (fmt_status == 0)
				{
					if (m_pattern[n] == '{')
					{
						str = m_pattern.substr(i + 1, n - i - 1);
						// std::cout << "*" << str << std::endl;
						fmt_status = 1; // 解析格式
						fmt_begin = n;
						++n;
						continue;
					}
				}
				else if (fmt_status == 1)
				{
					if (m_pattern[n] == '}')
					{
						fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
						// std::cout << "#" << fmt << std::endl;
						fmt_status = 0;
						++n;
						break;
					}
				}
				++n;
				if (n == m_pattern.size())
				{
					if (str.empty())
					{
						str = m_pattern.substr(i + 1);
					}
				}
			}

			if (fmt_status == 0)
			{
				if (!nstr.empty())
				{
					vec.push_back(std::make_tuple(nstr, std::string(), 0));
					nstr.clear();
				}
				vec.push_back(std::make_tuple(str, fmt, 1));
				i = n - 1;
			}
			else if (fmt_status == 1)
			{
				std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
				m_error = true;
				vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
			}
		}

		if (!nstr.empty())
		{
			vec.push_back(std::make_tuple(nstr, "", 0));
		}
		static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> s_format_items = {
#define XX(str, C)                                                               \
	{                                                                            \
		#str, [](const std::string &fmt) { return FormatItem::ptr(new C(fmt)); } \
	}

			XX(m, MessageFormatItem),  // m:消息
			XX(p, LevelFormatItem),	   // p:日志级别
			XX(r, ElapseFormatItem),   // r:累计毫秒数
			XX(c, NameFormatItem),	   // c:日志名称
			XX(t, ThreadIdFormatItem), // t:线程id
			XX(n, NewLineFormatItem),  // n:换行
			XX(d, DateTimeFormatItem), // d:时间
			XX(f, FilenameFormatItem), // f:文件名
			XX(l, LineFormatItem),	   // l:行号
			XX(T, TabFormatItem),	   // T:Tab
			XX(F, FiberIdFormatItem),  // F:协程id
#undef XX
		};

		for (auto &i : vec)
		{
			if (std::get<2>(i) == 0)
			{
				m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
			}
			else
			{
				auto it = s_format_items.find(std::get<0>(i));
				if (it == s_format_items.end())
				{
					m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
					m_error = true;
				}
				else
				{
					m_items.push_back(it->second(std::get<1>(i)));
				}
			}

			// std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
		}
		// std::cout << m_items.size() << std::endl;
	}

	LogManager::LogManager()
	{
		m_root.reset(new Logger);
		m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
		init();
	}
	Logger::ptr LogManager::getLogger(const std::string &name)
	{
		auto it = m_loggers.find(name);
		if(it != m_loggers.end())
		{
			return it->second;
		}
		//不存在就创建一个logger
		Logger::ptr logger(new Logger(name));
		logger->m_root = m_root;
		m_loggers[name] = logger;
		return logger;
	}

	struct LogAppenderDefine
	{
		int type = 0; //1是file  2是Stdout
		LogLevel::Level level = LogLevel::UNKONW;
		std::string formatter;
		std::string file;
		bool operator==(const LogAppenderDefine& oth) const
		{
			return oth.type == type && oth.level == level && oth.formatter ==formatter && oth.file == file;
		}
	};
	struct LogDefine
	{
		std::string name;
		LogLevel::Level level = LogLevel::UNKONW;
		std::string formatter;
		std::vector<LogAppenderDefine> appenders;
		bool operator==(const LogDefine& oth) const
		{
			return oth.name == name && oth.level == level && oth.formatter ==formatter && oth.appenders == appenders;
		}
		bool operator<(const LogDefine& oth) const
		{
			return name < oth.name;
		}

		bool isValid() const
		{
			return !name.empty();
		}
	};

	template <>
    class MyLexicalCast<std::string, LogDefine>
    {
    public:
        // string -> std::set<LogDefine>> 类型转化
        LogDefine operator()(const std::string& v)
        {
            YAML::Node n = YAML::Load(v);
            LogDefine ld;
			if(!n["name"].IsDefined())
			{
				std::cout << "log config error: name is null " << n << std::endl;
				throw std::logic_error("log config name is null");
			}
			ld.name = n["name"].as<std::string>();
			ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
			if(n["formatter"].IsDefined())
			{
				ld.formatter = n["formatter"].as<std::string>();
			}

			if(n["appenders"].IsDefined())
				{
				for(size_t x = 0;x<n["appenders"].size();++x)
				{
					auto a = n["appenders"][x];
					if(!a["type"].IsDefined())
					{
						std::cout << "log config error: appender type is null " << n << std::endl;
						continue;
					}
					std::string type = a["type"].as<std::string>();
					LogAppenderDefine lad;
					if(type == "FileLogAppender")
					{
						lad.type = 1;
						if(!a["file"].IsDefined())///!n["file"].IsDefined()
						{
							std::cout << "log config error: fileappender file is null " << a << std::endl;
							continue;
						}
						lad.file = a["file"].as<std::string>();
						if(a["formatter"].IsDefined())
						{
							lad.formatter = a["formatter"].as<std::string>();
						}
						
					}
					else if(type == "StdLogAppender")
					{
						lad.type = 2;
						if(a["formatter"].IsDefined())
						{
							lad.formatter = a["formatter"].as<std::string>();
						}
					}
					else
					{
						std::cout << "log config error: name is invalid " << n << std::endl;
						continue;
					}
					ld.appenders.push_back(lad);
				}
            }
            return ld;
        }
    };
    template <>
    class MyLexicalCast<LogDefine, std::string>
    {
    public:
        // std::set<LogDefine>> -> string 类型转化
        std::string operator()(const LogDefine& i)
        {
            YAML::Node n;
			n["name"] = i.name;
			if(i.level != LogLevel::UNKONW)
			{
				n["level"] = LogLevel::ToString();
			}
			if(!i.formatter.empty())
			{
				n["formatter"] = i.formatter;
			}

			for(auto& a: i.appenders)
			{
				YAML::Node na;
				if(a.type == 1)
				{
					na["type"] = "FileLogAppender";
					na["file"] = a.file;
				}
				else if(a.type == 2)
				{
					na["type"] = "StdLogAppender";
				}
				if(a.level)
			}
        }
    };

	yuan::ConfigVar<std::set<LogDefine>> g_log_defines = yuan::Config::Lookup("logs",std::set<LogDefine>(),"logs config");
	struct LogIniter
	{
		//在main函数之前执行
		LogIniter()
		{
			g_log_defines->addListener(0xF1E231,[](const std::set<LogDefine>& old_value,const std::set<LogDefine>& new_value)
			{
				YUAN_LOG_NAME(YUAN_LOG_ROOT()) << "on_logger_conf_changed";
				for(auto& i : new_value)
				{
					auto it = old_value.find(i);
					yuan::Logger::ptr logger;
					if(it == old_value.end())//新增logger
					{
						logger.reset(new yuan::Logger(i.name));
						
					}
					else
					{
						if(!(i == *it))//新的和旧的不相等，修改logger  i是新的LogDefine
						{
							logger = YUAN_LOG_NAME(i.name);
						}
					}
					logger->setLevel(i.level);
					if(!i.formatter.empty())
					{
						logger->setFormatter(i.formatter);
					}
					logger->clearAppender();
					for(auto& a : i.appenders)
					{
						yuan::LogAppender::ptr ap;
						if(a.type == 1)
						{
							ap.reset(new FileLogAppender(a.file));
						}
						else if(a.type == 2)
						{
							ap.reset(new StdoutLogAppender);
						}
						ap->setLevel(a.level);
						logger->addAppender(ap);
					}
				}

				//删除
				for(auto& i : old_value)
				{
					auto it = new_value.find(i);
					if(it == new_value.end())//删除logger
					{
						auto logger = YUAN_LOG_NAME(i.name);
						logger->setLevel((LogLevel::Level)100);//将级别设的很高
						logger->clearAppender();
					}
				}
			});
		}
	}
	static LogIniter __log_init;
	void LogManager::init()
	{

	}

}