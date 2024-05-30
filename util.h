#ifndef UTIL_H
#define UTIL_H

#include <sys/types.h>
#include <stdint.h>
#include <sys/time.h>
#include <cxxabi.h> // abi::__cxa_demangle()
#include <string>
#include <vector>
#include <iostream>

namespace MyServer
{
    /**
     * @brief 获取线程id
     */
    pid_t GetThreadId();

    /**
     * @brief 获取协程id
     */
    uint64_t GetFiberId();

    /**
     * @brief 获取当前启动的毫秒数
     */
    uint64_t GetElapsedMS();

    /**
     * @brief 获取线程名称
     */
    std::string GetThreadName();

    /**
     * @brief 设置线程名称
     * @note 线程名称不能超过16字节，包括结尾的'\0'字符
     */
    void SetThreadName(const std::string &name);

    /**
     * @brief 获取当前的调用栈
     * @param[out] bt 保存调用栈
     * @param[in] size 最多返回层数
     * @param[in] skip 跳过栈顶的层数
     */
    void Backtrace(std::vector<std::string> &bt, int size = 64, int skip = 1);

    /**
     * @brief 获取当前栈信息的字符串
     * @param[in] size 栈的最大层数
     * @param[in] skip 跳过栈顶的层数
     * @param[in] prefix 栈信息前输出的内容
     */
    std::string BacktraceToString(int size = 64, int skip = 2, const std::string &prefix = "");

    /**
     * @brief 获取当前时间的毫秒
     */
    uint64_t GetCurrentMS();

    /**
     * @brief 获取当前时间的微秒
     */
    uint64_t GetCurrentUS();

    /**
     * @brief 字符串转大写
     */
    std::string ToUpper(const std::string &name);

    /**
     * @brief 字符串转小写
     */
    std::string ToLower(const std::string &name);

    /**
     * @brief 日期时间转字符串
     */
    std::string Time2Str(time_t ts = time(0), const std::string &format = "%Y-%m-%d %H:%M:%S");

    /**
     * @brief 字符串转日期时间
     */
    time_t Str2Time(const char *str, const char *format = "%Y-%m-%d %H:%M:%S");

    

} // namespace MyServer

#endif