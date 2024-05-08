#include "log.h"

namespace MyServer
{
    Logger::Logger(const std::string& name)
    :m_name(name)
    {

    }

    void Logger::log(Level level, LogEvent::ptr event)
    {

    }

    void Logger::debug(LogEvent::ptr event)
    {

    }

    void Logger::info(LogEvent::ptr event)
    {

    }

    void Logger::warn(LogEvent::ptr event)
    {

    }

    void Logger::error(LogEvent::ptr event)
    {

    }

    void Logger::addAppender(LogAppender::ptr appender)
    {
        m_appenders.push_back(appender);
    }

    void Logger::delAppender(LogAppender::ptr appender)
    {
        for(auto it=m_appenders.begin();it!=m_appenders.end();it++)
        {
            if(*it==appender)
            {
                m_appenders.erase(it);
                break;
            }
        }
    }

    void StdoutLogAppender::log(Level level, LogEvent::ptr event)
    {
        if(level>=m_level)
        {
            std::cout<< m_formatter->format(event);
        }
    }

    void FileLogAppender::log(Level level, LogEvent::ptr event)
    {
        if(level>=m_level)
        {
            m_filestream << m_formatter->format(event);
        }
    }

    bool FileLogAppender::reopen()
    {
        if(m_filestream)
        {
            m_filestream.close();
        }
        m_filestream.open(m_filename);
        return !!m_filestream;
    }

} // end MyServer