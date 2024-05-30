#include "log.h"

namespace MyServer
{
    const char *LogLevel::ToString(LogLevel::Level level)
    {
        switch (level)
        {
#define XX(name)         \
    case LogLevel::name: \
        return #name;
            XX(FATAL);
            XX(ALERT);
            XX(CRIT);
            XX(ERROR);
            XX(WARN);
            XX(NOTICE);
            XX(INFO);
            XX(DEBUG);
#undef XX
        default:
            return "UNKNOW";
        }
        return "UNKNOW";
    }

    LogLevel::Level LogLevel::FromString(const std::string &str)
    {
#define XX(level, v)            \
    if (str == #v)              \
    {                           \
        return LogLevel::level; \
    }
        XX(FATAL, fatal);
        XX(ALERT, alert);
        XX(CRIT, crit);
        XX(ERROR, error);
        XX(WARN, warn);
        XX(NOTICE, notice);
        XX(INFO, info);
        XX(DEBUG, debug);

        XX(FATAL, FATAL);
        XX(ALERT, ALERT);
        XX(CRIT, CRIT);
        XX(ERROR, ERROR);
        XX(WARN, WARN);
        XX(NOTICE, NOTICE);
        XX(INFO, INFO);
        XX(DEBUG, DEBUG);
#undef XX
        return LogLevel::UNKNOW;
    }

    LogEvent::LogEvent(const std::string &logger_name, LogLevel::Level level, const char *file, int32_t line,
                       int64_t elapse, uint32_t thread_id, uint64_t fiber_id, time_t time, const std::string &thread_name)
        : m_level(level), m_file(file), m_line(line), m_elapse(elapse), m_threadId(thread_id), m_fiberId(fiber_id), m_time(time), m_threadName(thread_name), m_loggerName(logger_name)
    {
    }

    void LogEvent::printf(const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        vprintf(fmt, ap);
        va_end(ap);
    }

    void LogEvent::vprintf(const char *fmt, va_list ap)
    {
        char *buf = nullptr;
        int len = vasprintf(&buf, fmt, ap);
        if (len != -1)
        {
            m_ss << std::string(buf, len);
            free(buf);
        }
    }

    class MessageFormatItem : public LogFormatter::FormatItem
    {
    public:
        MessageFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getContent();
        }
    };

    class LevelFormatItem : public LogFormatter::FormatItem
    {
    public:
        LevelFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << LogLevel::ToString(event->getLevel());
        }
    };

    class ElapseFormatItem : public LogFormatter::FormatItem
    {
    public:
        ElapseFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getElapse();
        }
    };

    class LoggerNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        LoggerNameFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getLoggerName();
        }
    };

    class ThreadIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadIdFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getThreadId();
        }
    };

    class FiberIdFormatItem : public LogFormatter::FormatItem
    {
    public:
        FiberIdFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getFiberId();
        }
    };

    class ThreadNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        ThreadNameFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getThreadName();
        }
    };

    class DateTimeFormatItem : public LogFormatter::FormatItem
    {
    private:
        std::string m_format;

    public:
        DateTimeFormatItem(const std::string &format = "%Y-%m-%d %H:%M:%S")
            : m_format(format)
        {
            if (format.empty())
                m_format = "%Y-%m-%d %H:%M:%S";
        }

        void format(std::ostream &os, LogEvent::ptr event) override
        {
            struct tm tm;
            time_t time = event->getTime();
            localtime_r(&time, &tm); // 线程安全的将时间戳转换为tm结构体
            char buf[64];
            strftime(buf, sizeof(buf), m_format.c_str(), &tm); // 根据m_format格式将tm中保存的信息放入buf
            os << buf;
        }
    };

    class FileNameFormatItem : public LogFormatter::FormatItem
    {
    public:
        FileNameFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getFile();
        }
    };

    class LineFormatItem : public LogFormatter::FormatItem
    {
    public:
        LineFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << event->getLine();
        }
    };

    class NewLineFormatItem : public LogFormatter::FormatItem
    {
    public:
        NewLineFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << std::endl;
        }
    };

    class StringFormatItem : public LogFormatter::FormatItem
    {
    public:
        StringFormatItem(const std::string &str) : m_string(str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << m_string;
        }

    private:
        std::string m_string;
    };

    class TabFormatItem : public LogFormatter::FormatItem
    {
    public:
        TabFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << "\t";
        }
    };

    class PercentSignFormatItem : public LogFormatter::FormatItem
    {
    public:
        PercentSignFormatItem(const std::string &str) {}
        void format(std::ostream &os, LogEvent::ptr event) override
        {
            os << "%";
        }
    };

    LogFormatter::LogFormatter(const std::string &pattern)
        : m_pattern(pattern)
    {
        init();
    }

    /*
    使用状态机进行判断，提取pattern中的字符
    */
    void LogFormatter::init()
    {
        std::vector<std::pair<bool, std::string>> patterns; // patterns存储的每个pattern包含一个bool类型和string，0表示常规字符串，1表示需要转义
        std::string tmp;                                    // 存储常规字符串
        std::string dateformat;                             // 存储日期格式
        bool parsing_string = true;                         // 是否解析常规字符

        for (size_t i = 0; i < m_pattern.size(); i++)
        {
            std::string c = std::string(1, m_pattern[i]);
            if (c == "%")
            {
                if (parsing_string)
                {
                    if (!tmp.empty())
                    {
                        patterns.push_back(std::make_pair(0, tmp));
                        tmp.clear();
                    }
                    parsing_string = false;
                }
                else
                {
                    patterns.push_back(std::make_pair(1, c));
                    parsing_string = true;
                }
            }
            else
            {
                if (parsing_string)
                {
                    tmp += c;
                }
                else
                {
                    patterns.push_back(std::make_pair(1, c));
                    parsing_string = true;
                    if (c == "d")
                    {
                        if (++i < m_pattern.size() && c != "{")
                            continue;
                        while (++i < m_pattern.size() && c != "}")
                        {
                            dateformat += c;
                        }
                        if (i > m_pattern.size())
                        {
                            std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] '{' not closed" << std::endl;
                            m_error = true;
                            return;
                        }
                    }
                }
            }
        }

        if (!tmp.empty())
            patterns.push_back(std::make_pair(0, std::move(tmp)));
        static std::map<std::string, std::function<FormatItem::ptr(const std::string &str)>> format_items =
            {
#define XX(str, c)                                                               \
    {                                                                            \
        #str, [](const std::string &fmt) { return FormatItem::ptr(new c(fmt)); } \
    }
                XX(m, MessageFormatItem),     // m:消息
                XX(p, LevelFormatItem),       // p:日志级别
                XX(c, LoggerNameFormatItem),  // c:日志器名称
                XX(d, DateTimeFormatItem),    // d:日期时间
                XX(r, ElapseFormatItem),      // r:累计毫秒数
                XX(f, FileNameFormatItem),    // f:文件名
                XX(l, LineFormatItem),        // l:行号
                XX(t, ThreadIdFormatItem),    // t:编程号
                XX(F, FiberIdFormatItem),     // F:协程号
                XX(N, ThreadNameFormatItem),  // N:线程名称
                XX(%, PercentSignFormatItem), // %:百分号
                XX(T, TabFormatItem),         // T:制表符
                XX(n, NewLineFormatItem),     // n:换行符
#undef XX
            };

        for (auto &v : patterns)
        {
            if (v.first == 0)
            {
                m_items.push_back(FormatItem::ptr(new StringFormatItem(v.second)));
            }
            // else if(v.second=="d")
            // {
            //     m_items.push_back(FormatItem::ptr(new DateTimeFormatItem(v.second)))
            // }
            else
            {
                auto it = format_items.find(v.second);
                if (it == format_items.end())
                {
                    std::cout << "[ERROR] LogFormatter::init() " << "pattern: [" << m_pattern << "] " << "unknown format item: " << v.second << std::endl;
                    m_error = true;
                    return;
                }
                else
                {
                    m_items.push_back(it->second(v.second));
                }
            }
        }
    }

    std::string LogFormatter::format(LogEvent::ptr event)
    {
        std::stringstream ss;
        for (auto &i : m_items)
        {
            i->format(ss, event);
        }
        return ss.str();
    }

    std::ostream &LogFormatter::format(std::ostream &os, LogEvent::ptr event)
    {
        for (auto &i : m_items)
        {
            i->format(os, event);
        }
        return os;
    }

    void LogAppender::setFormatter(LogFormatter::ptr fmt)
    {
        MutexType::Lock lock(m_mutex);
        m_formatter = fmt;
    }

    LogFormatter::ptr LogAppender::getFormatter()
    {
        MutexType::Lock lock(m_mutex);
        return m_formatter ? m_formatter : m_defaultformatter;
    }

    void StdoutLogAppender::log(LogEvent::ptr event)
    {
        getFormatter()->format(std::cout, event);
    }

    std::string StdoutLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "StdoutLogAppender";
        node["pattern"] = m_formatter->getPattern();
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    FileLogAppender::FileLogAppender(const std::string &file)
        : m_filename(file), LogAppender(LogFormatter::ptr(new LogFormatter))
    {
        if (!reopen())
            std::cout << "reopen file " << m_filename << " error" << std::endl;
    }

    bool FileLogAppender::reopen()
    {
        MutexType::Lock lock(m_mutex);
        if (m_filestream)
            m_filestream.close();
        m_filestream.open(m_filename, std::ios::app);
        m_reopenError = !m_filestream;
        return !m_reopenError;
    }

    void FileLogAppender::log(LogEvent::ptr event)
    {
        time_t now = event->getTime();
        if (now >= (m_lastTime + 3)) // 如果一个日志事件距离上次写日志超过3秒，那就重新打开一次日志文件
        {
            if (!reopen())
            {
                std::cout << "reopen file " << m_filename << " error" << std::endl;
                return;
            }
        }
        m_lastTime = now;
        MutexType::Lock lock(m_mutex);
        if (m_formatter)
        {
            if (!m_formatter->format(m_filestream, event))
            {
                std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
            }
        }
        else
        {
            if (!m_defaultformatter->format(m_filestream, event))
            {
                std::cout << "[ERROR] FileLogAppender::log() format error" << std::endl;
            }
        }
    }

    std::string FileLogAppender::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["type"] = "FileLogAppender";
        node["file"] = m_filename;
        node["pattern"] = m_formatter ? m_formatter->getPattern() : m_defaultformatter->getPattern();
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    Logger::Logger(const std::string &name)
        : m_name(name), m_level(LogLevel::INFO), m_createTime(GetElapsedMS())
    {
    }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        MutexType::Lock lock(m_mutex);
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender)
    {
        MutexType::Lock lock(m_mutex);
        for (auto it = m_appenders.begin(); it != m_appenders.end(); it++)
        {
            if (*it == appender)
            {
                m_appenders.erase(it);
                return;
            }
        }
    }

    void Logger::clearAppenders()
    {
        MutexType::Lock lock(m_mutex);
        m_appenders.clear();
    }

    void Logger::log(LogEvent::ptr event)
    {
        if (event->getLevel() <= m_level)
        {
            for (auto &i : m_appenders)
            {
                i->log(event);
            }
        }
    }

    std::string Logger::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        node["name"] = m_name;
        node["level"] = LogLevel::ToString(m_level);
        for (auto &i : m_appenders)
        {
            node["appenders"].push_back(YAML::Load(i->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }

    LogEventWrap::LogEventWrap(Logger::ptr logger, LogEvent::ptr event)
        : m_logger(logger), m_event(event)
    {
    }

    LogEventWrap::~LogEventWrap()
    {
        m_logger->log(m_event);
    }

    LoggerManager::LoggerManager()
        : m_root(new Logger("root"))
    {
        m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
        m_loggers[m_root->getName()] = m_root;
        init();
    }

    Logger::ptr LoggerManager::getLogger(const std::string &name)
    {
        MutexType::Lock lock(m_mutex);
        auto it = m_loggers.find(name);
        if (it != m_loggers.end())
        {
            return it->second;
        }

        Logger::ptr logger(new Logger(name));
        m_loggers[name] = logger;
        return logger;
    }

    std::string LoggerManager::toYamlString()
    {
        MutexType::Lock lock(m_mutex);
        YAML::Node node;
        for (auto &i : m_loggers)
        {
            node.push_back(YAML::Load(i.second->toYamlString()));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
} // end MyServer