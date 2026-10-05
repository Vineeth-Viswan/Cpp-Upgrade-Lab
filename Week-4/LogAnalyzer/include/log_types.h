#pragma once

#ifndef LOG_TYPES
#define LOG_TYPES

#include <string_view>
#include <optional>
#include <string>

enum class LogLevel
{
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

std::string_view ToString (LogLevel level)
{
    switch (level)
    {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
};

std::optional<LogLevel> FromString (std::string_view str)
{
    if (str == "DEBUG")
        return LogLevel::DEBUG;
    else if (str == "INFO")
        return LogLevel::INFO;
    else if (str == "WARNING")
        return LogLevel::WARNING;
    else if (str == "ERROR")
        return LogLevel::ERROR;
    else
        return std::nullopt;
}

struct RawLine
{
    size_t line_number;
    std::string content;
};

struct LogEntry
{
    LogLevel level;
    std::string timestamp;
    std::string source;
    std::string message;
    size_t line_number;
};

#endif // !LOG_TYPES
