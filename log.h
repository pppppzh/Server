#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <map>

namespace MyServer
{
    class LogLevel // 日志级别
    {
    public:
        enum Level
        {
            UNKNOW,
            DEBUG,
            INFO,
            WARN,
            ERROR,
            FATAL
        };

        static const char *LevelToString(LogLevel::Level);
        static LogLevel::Level StringToLevel(const std::string &);
    };

    class LogEvent // 日志事件
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;

        LogEvent(Logger::ptr logger, LogLevel::Level level, const char *file,
                 uint32_t line, uint32_t elapse, uint32_t thread_id,
                 uint32_t fiber_id, uint64_t time, const std::string &thread_name);

        const char *getFile() const { return m_file; }
        uint32_t getLine() const { return m_line; }
        uint32_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint32_t getFiberId() const { return m_fiberId; }
        uint64_t getTime() const { return m_time; }
        const std::string &getThreadName() const { return m_threadName; }
        std::string getContent() const { return m_ss.str(); }
        std::shared_ptr<Logger> getLogger() const { return m_logger; }
        LogLevel::Level getLevel() const { return m_level; }
        std::stringstream &getSS() { return m_ss; }
        void format(const char *fmt, ...);        // 格式化写入日志内容
        void format(const char *fmt, va_list al); // 格式化写入日志内容
    private:
        const char *m_file = nullptr;
        uint32_t m_line = 0;
        uint32_t m_threadId = 0;          // 线程id
        uint32_t m_fiberId = 0;           // 协程id
        uint32_t m_elapse = 0;            // 程序启动开始到现在的毫秒数
        uint64_t m_time = 0;              // 时间戳
        std::string m_threadName;         // 线程名称
        std::stringstream m_ss;           // 日志内容流
        std::shared_ptr<Logger> m_logger; // 日志器
        LogLevel::Level m_level;          // 日志等级
    };

    class LogEventWrap
    {
    public:
        LogEventWrap(LogEvent::ptr);
        LogEvent::ptr getEvent() const { return m_event; }
        std::stringstream &getSS();

    private:
        LogEvent::ptr m_event;
    };

    class Logger : public std::enable_shared_from_this<Logger> // 日志输出器
    {
        friend class LoggerManager;

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
        void clearAppenders();
        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level l) { m_level = l; }
        const std::string &getName() const { return m_name; }
        void setFormatter(LogFormatter::ptr val);
        void setFormatter(const std::string &val);
        LogFormatter::ptr getFormatter();
        std::string toYamlString();

    private:
        std::string m_name;
        LogLevel::Level m_level;
        std::list<LogAppender::ptr> m_appenders;
        Spinlock m_mtx;
        LogFormatter::ptr m_formatter;
        Logger::ptr m_root;
    };

    class LogFormatter // 日志格式
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        LogFormatter(const std::string &);
        std::string format(Logger::ptr, LogLevel::Level, LogEvent::ptr);
        std::ostream format(std::ostream &, Logger::ptr, LogLevel::Level, LogEvent::ptr);

        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            virtual ~FormatItem() {}
            virtual void format(std::ostream, Logger::ptr, LogLevel::Level, LogEvent::ptr) = 0;
        };

        void init();
        bool isError() const { return m_error; }

    private:
        std::string m_pattern;
        std::vector<FormatItem::ptr> m_items;
        bool m_error = false;
    };

    class LogAppender // 日志输出地
    {
        friend class Logger;

    public:
        typedef std::shared_ptr<LogAppender> ptr;
        virtual ~LogAppender();
        virtual void log(Logger::ptr, LogLevel::Level level, LogEvent::ptr event) = 0;
        virtual std::string toYamlString() = 0;
        void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
        LogFormatter::ptr getFormatter() { return m_formatter; }
        LogLevel::Level getLevel() const { return m_level; }
        void setLevel(LogLevel::Level val) { m_level = val; }

    protected:
        LogLevel::Level m_level = LogLevel::UNKNOW;
        bool m_hasFormatter = false;
        LogFormatter::ptr m_formatter;
        Spinlock m_mtx;
    };

    class StdoutLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        void log(Logger::ptr, LogLevel::Level level, LogEvent::ptr event) override;

    private:
    };

    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string &filename);
        void log(Logger::ptr, LogLevel::Level level, LogEvent::ptr event) override;
        bool reopen();

    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };

    class LoggerManager
    {
    public:
        LoggerManager();
        Logger::ptr getLogger(const std::string &name);
        void init();
        Logger::ptr getRoot() const { return m_root; }
        std::string toYamlString();

    private:
        Spinlock m_mutex;
        std::map<std::string, Logger::ptr> m_loggers;
        Logger::ptr m_root;
    };

    /// 日志器管理类单例模式
    typedef MyServer::Singleton<LoggerManager> LoggerMgr;

} // end MyServer

#endif