#ifndef LOG_H
#define LOG_H

#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <sstream>
#include <sstream>
#include <fstream>
#include <vector>
#include <stdarg.h>
#include <map>
#include <vector>
#include <stdarg.h>
#include <map>

namespace MyServer
{
    class LogLevel
    {
    public:
        enum Level
        {
            // 致命情况，系统不可用
            FATAL = 0,
            // 高优先级情况，例如数据库系统崩溃
            ALERT = 100,
            // 严重错误，例如硬盘错误
            CRIT = 200,
            // 错误
            ERROR = 300,
            // 警告
            WARN = 400,
            // 正常但值得注意
            NOTICE = 500,
            // 一般信息
            INFO = 600,
            // 调试信息
            DEBUG = 700,
            // 未知
            UNKNOW = 800,
        };

        /*
        @brief 日志级别转字符串
        @param level 日志级别
        @return 字符串形式日志级别
        */
        static const char *ToString(LogLevel::Level level);

        /*
        @brief 字符串转日志级别
        @param str 字符串
        @return 日志级别
        */
        static LogLevel::Level FromString(const std::string &str);
    };

    /*
    @brief 日志事件，记录日志现场
    */
    class LogEvent
    {
    private:
        LogLevel::Level m_level;      // 日志级别
        std::stringstream m_ss;       // 日志内容
        const char *m_file = nullptr; // 文件名
        int32_t m_line = 0;           // 行号
        int64_t m_elapse = 0;         // 日志创建到当前的耗时
        uint32_t m_threadId = 0;      // 线程id
        uint64_t m_fiberId = 0;       // 协程id
        time_t m_time;                // UTC时间戳
        std::string m_threadName;     // 线程名称
        std::string m_loggerName;     // 日志器名称
    public:
        typedef std::shared_ptr<LogEvent> ptr;

        /*
        @param[in] logger_name 日志器名称
        @param[in] level 日志级别
        @param[in] file 文件名
        @param[in] line 行号
        @param[in] elapse 从日志器创建开始到当前的累计运行毫秒
        @param[in] thead_id 线程id
        @param[in] fiber_id 协程id
        @param[in] time UTC时间
        @param[in] thread_name 线程名称
        */
        LogEvent(const std::string &logger_name, LogLevel::Level level, const char *file, int32_t line,
                 int64_t elapse, uint32_t thread_id, uint64_t fiber_id, time_t time, const std::string &thread_name);

        LogLevel::Level getLevel() const { return m_level; }
        std::string getContent() const { return m_ss.str(); }
        std::string getFile() const { return m_file; }
        int32_t getLine() const { return m_line; }
        int64_t getElapse() const { return m_elapse; }
        uint32_t getThreadId() const { return m_threadId; }
        uint64_t getFiberId() const { return m_fiberId; }
        time_t getTime() const { return m_time; }
        const std::string &getThreadName() const { return m_threadName; }
        std::stringstream &getSS() { return m_ss; }
        const std::string &getLoggerName() const { return m_loggerName; }

        /*
        @brief printf风格写入日志
        */
        void printf(const char *fmt, ...);

        /*
        @brief vprintf风格写入日志
        */
        void vprintf(const char *fmt, va_list ap);
    };

    class LogFormatter
    {
    public:
        typedef std::shared_ptr<LogFormatter> ptr;

        /*
        @param[in] pattern 格式模板
        @details 模板参数说明：
      - %%m 消息
      - %%p 日志级别
      - %%c 日志器名称
      - %%d 日期时间，后面可跟一对括号指定时间格式，比如%%d{%%Y-%%m-%%d %%H:%%M:%%S}，这里的格式字符与C语言strftime一致
      - %%r 该日志器创建后的累计运行毫秒数
      - %%f 文件名
      - %%l 行号
      - %%t 线程id
      - %%F 协程id
      - %%N 线程名称
      - %%% 百分号
      - %%T 制表符
      - %%n 换行

      默认格式：%%d{%%Y-%%m-%%d %%H:%%M:%%S}%%T%%t%%T%%N%%T%%F%%T[%%p]%%T[%%c]%%T%%f:%%l%%T%%m%%n

      默认格式描述：年-月-日 时:分:秒 [累计运行毫秒数] \\t 线程id \\t 线程名称 \\t 协程id \\t [日志级别] \\t [日志器名称] \\t 文件名:行号 \\t 日志消息 换行符
        */
        LogFormatter(const std::string &pattern = "%d{%Y-%m-%d %H:%M:%S} [%rms]%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n");

        /*
        @brief 初始化，解析格式模板
        */
        void init();

        /*
         @brief 解析格式模板是否出错
         */
        bool isError() const { return m_error; }

        /*
        @brief 对日志事件格式化，返回格式化文本
        */
        std::string format(LogEvent::ptr event);

        /*
         @brief 对日志事件格式化，返回格式化日志流
         */
        std::ostream &format(std::ostream &os, LogEvent::ptr event);

        std::string getPattern() const { return m_pattern; }

        class FormatItem
        {
        public:
            typedef std::shared_ptr<FormatItem> ptr;
            virtual ~FormatItem();

            /*
            @brief 格式化日志事件
            */
            virtual void format(std::ostream &os, LogEvent::ptr event) = 0;
        };
        
    private:
        std::string m_pattern;                // 日志格式模板
        std::vector<FormatItem::ptr> m_items; // 解析后格式模板数组
        bool m_error = false;                 // 是否出错
    };

    


} // end MyServer

#endif