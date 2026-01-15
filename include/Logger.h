/**
 * @file Logger.h
 * @brief 日志系统 - 用于调试和问题排查
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <windows.h>

/**
 * @brief 日志级别
 */
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    LOG_ERROR  // 避免与Windows ERROR宏冲突
};

/**
 * @brief 日志系统类（单例模式）
 *
 * 功能：
 * - 输出到debug.log文件
 * - 支持多种日志级别
 * - 自动添加时间戳
 * - 线程安全
 * - UTF-8编码
 */
class Logger {
public:
    /**
     * @brief 获取Logger单例
     */
    static Logger& GetInstance();

    /**
     * @brief 写入日志
     * @param level 日志级别
     * @param message 日志消息（UTF-8编码）
     */
    void Log(LogLevel level, const std::string& message);

    /**
     * @brief 写入DEBUG级别日志
     */
    void Debug(const std::string& message);

    /**
     * @brief 写入INFO级别日志
     */
    void Info(const std::string& message);

    /**
     * @brief 写入WARNING级别日志
     */
    void Warning(const std::string& message);

    /**
     * @brief 写入ERROR级别日志
     */
    void Error(const std::string& message);

    /**
     * @brief 刷新日志缓冲区
     */
    void Flush();

private:
    Logger();
    ~Logger();

    // 禁止拷贝
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief 获取当前时间字符串
     */
    std::string GetCurrentTime();

    /**
     * @brief 获取日志级别字符串
     */
    std::string GetLevelString(LogLevel level);

    std::ofstream m_logFile;
    std::mutex m_mutex;
};

// 便捷宏定义
#define LOG_DEBUG(msg) Logger::GetInstance().Debug(msg)
#define LOG_INFO(msg) Logger::GetInstance().Info(msg)
#define LOG_WARNING(msg) Logger::GetInstance().Warning(msg)
#define LOG_ERROR(msg) Logger::GetInstance().Error(msg)

#endif  // LOGGER_H
