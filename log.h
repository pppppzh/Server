#ifndef _LOG_H
#define _LOG_H

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <fstream>
#include <sstream>
#include <iostream>

namespace MyServer
{
    enum class Level
    {
        DEBUG,
        INFO,
        WARN,
        ERROR,
    };

    class LogEvent
    {
    public:
        typedef std::shared_ptr<LogEvent> ptr;

        LogEvent();

    private:
        const char *m_file = nullptr;
        int32_t m_line = 0;
        uint32_t m_threadId = 0; // 线程id
        uint32_t m_fiberId = 0;  // 协程id
        uint32_t m_elapse = 0;
        uint64_t m_time = 0;
        std::string m_content;
    };

    class Logger // 日志输出器
    {
    public:
        typedef std::shared_ptr<Logger> ptr;
        Logger(const std::string &name = "root");
        void log(Level level, LogEvent::ptr event);

        void debug(LogEvent::ptr event);
        void info(LogEvent::ptr event);
        void warn(LogEvent::ptr event);
        void error(LogEvent::ptr event);

        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        Level getLevel() const { return m_level; }
        void setLevel(Level l) { m_level = l; }

    private:
        std::string m_name;
        Level m_level;
        std::list<LogAppender::ptr> m_appenders;
    };

    class LogAppender // 日志输出地
    {
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        virtual ~LogAppender();
        virtual void log(Level level, LogEvent::ptr event) = 0;
        void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
        LogFormatter::ptr getFormatter() { return m_formatter; }

    protected:
        Level m_level;
        LogFormatter::ptr m_formatter;
    };

    class LogFormatter // 日志格式
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;
        std::string format(LogEvent::ptr event);
    private:
        
    };

    class StdoutLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<StdoutLogAppender> ptr;
        void log(Level level, LogEvent::ptr event) override;

    private:
    };

    class FileLogAppender : public LogAppender
    {
    public:
        typedef std::shared_ptr<FileLogAppender> ptr;
        FileLogAppender(const std::string &filename);
        void log(Level level, LogEvent::ptr event) override;
        bool reopen();
    private:
        std::string m_filename;
        std::ofstream m_filestream;
    };

} // end MyServer

#endif