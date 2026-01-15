/**
 * @file Logger.cpp
 * @brief 日志系统实现
 */

#include "Logger.h"
#include <sstream>
#include <iomanip>
#include <ctime>

Logger& Logger::GetInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    // 打开日志文件（追加模式）
    m_logFile.open("debug.log", std::ios::out | std::ios::app | std::ios::binary);

    if (m_logFile.is_open()) {
        // 写入UTF-8 BOM
        const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
        m_logFile.write((const char*)bom, 3);

        // 写入启动标记
        std::string startMsg = "\n========== 程序启动 " + GetCurrentTime() + " ==========\n";
        m_logFile.write(startMsg.c_str(), startMsg.length());
        m_logFile.flush();
    }
}

Logger::~Logger() {
    if (m_logFile.is_open()) {
        std::string endMsg = "========== 程序退出 " + GetCurrentTime() + " ==========\n\n";
        m_logFile.write(endMsg.c_str(), endMsg.length());
        m_logFile.close();
    }
}

void Logger::Log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_logFile.is_open()) {
        return;
    }

    // 格式：[时间] [级别] 消息
    std::string logLine = "[" + GetCurrentTime() + "] [" + GetLevelString(level) + "] " + message + "\n";
    m_logFile.write(logLine.c_str(), logLine.length());
    m_logFile.flush();
}

void Logger::Debug(const std::string& message) {
    Log(LogLevel::DEBUG, message);
}

void Logger::Info(const std::string& message) {
    Log(LogLevel::INFO, message);
}

void Logger::Warning(const std::string& message) {
    Log(LogLevel::WARNING, message);
}

void Logger::Error(const std::string& message) {
    Log(LogLevel::LOG_ERROR, message);
}

void Logger::Flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_logFile.is_open()) {
        m_logFile.flush();
    }
}

std::string Logger::GetCurrentTime() {
    SYSTEMTIME st;
    GetLocalTime(&st);

    char buf[64];
    sprintf_s(buf, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
              st.wYear, st.wMonth, st.wDay,
              st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

    return std::string(buf);
}

std::string Logger::GetLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:     return "DEBUG";
        case LogLevel::INFO:      return "INFO ";
        case LogLevel::WARNING:   return "WARN ";
        case LogLevel::LOG_ERROR: return "ERROR";
        default:                  return "UNKN ";
    }
}
