#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>

#ifdef _MSC_VER
#include <io.h>
#else
#include <unistd.h>
#endif

class Logger
{
public:
    enum class Level
    {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3
    };

    /// @brief Set the minimum log level. Messages below this level are discarded.
    /// @param level The minimum level to log
    static void SetMinLevel (Level level) noexcept
    {
        std::lock_guard<std::mutex> lock (log_mutex);
        min_level = level;
    }

    /// @brief Get the current minimum log level
    /// @return The current minimum log level
    [[nodiscard]] static Level GetMinLevel () noexcept
    {
        std::lock_guard<std::mutex> lock (log_mutex);
        return min_level;
    }

    /// @brief Initialize logger from environment variables or config
    /// Sets up the minimum log level based on environment variables.
    /// Environment variables checked (in order of precedence):
    ///   - LOG_LEVEL: Can be "DEBUG", "INFO", "WARNING", "ERROR"
    ///   - DEBUG: If set, enables DEBUG level (overrides LOG_LEVEL)
    static void InitializeFromEnvironment () noexcept
    {
        // Platform-safe way to read environment variables
        auto getEnvSafe = [] (const char *varName) -> std::string
        {
#ifdef _MSC_VER
            char buffer[256] = {0};
            size_t size = 0;
            if (getenv_s (&size, buffer, sizeof (buffer), varName) == 0 && size > 0)
            {
                return std::string (buffer);
            }
            return {};
#else
            const char *val = std::getenv (varName);
            return val ? std::string (val) : std::string ();
#endif
        };

        // Check if DEBUG is explicitly set
        std::string debugVal = getEnvSafe ("DEBUG");
        if (!debugVal.empty ())
        {
            SetMinLevel (Level::DEBUG);
            return;
        }

        // Check LOG_LEVEL environment variable
        std::string logLevelVal = getEnvSafe ("LOG_LEVEL");
        if (!logLevelVal.empty ())
        {
            // Convert to lowercase for case-insensitive comparison
            std::string levelStr = logLevelVal;
            std::transform (levelStr.begin (), levelStr.end (), levelStr.begin (),
                    [] (unsigned char c) { return std::tolower (c); });

            if (levelStr == "debug")
                SetMinLevel (Level::DEBUG);
            else if (levelStr == "info")
                SetMinLevel (Level::INFO);
            else if (levelStr == "warning")
                SetMinLevel (Level::WARNING);
            else if (levelStr == "error")
                SetMinLevel (Level::ERROR);
        }
        // else: default is INFO (built-in default)
    }

    static void Log (Level level, std::string_view message)
    {
        std::lock_guard<std::mutex> lock (log_mutex);
        // Gate: check if message meets minimum level
        {
            if (static_cast<int> (level) < static_cast<int> (min_level))
                return;
        }
        auto now = std::chrono::system_clock::now ();
        auto timestamp = std::format ("{:%Y-%m-%d %H:%M:%S}", now);
        auto &stream = (level == Level::ERROR || level == Level::WARNING) ? std::cerr : std::cout;

        if (ShouldUseColor (level))
        {
            stream << std::format ("{}[{}] {}: {}{}\n",
                    GetLevelColor (level), // Start color
                    timestamp, GetLevelString (level), message,
                    "\033[0m" // Reset color at the end
            );
        }
        else
        {
            stream << std::format ("[{}] {}: {}\n", timestamp, GetLevelString (level), message);
        }
    }

    template<typename... Args>
    static void Log (Level level, std::format_string<Args...> fmt, Args &&...args)
    {
        Log (level, std::format (fmt, std::forward<Args> (args)...));
    }

private:
    static std::string_view GetLevelString (Level level)
    {
        switch (level)
        {
            case Level::DEBUG:
                return "DEBUG";
            case Level::INFO:
                return "INFO";
            case Level::WARNING:
                return "WARNING";
            case Level::ERROR:
                return "ERROR";
            default:
                return "UNKNOWN";
        }
    }

    static std::string_view GetLevelColor (Level level)
    {
        switch (level)
        {
            case Level::DEBUG:
                return "\033[35m"; // Magenta
            case Level::INFO:
                return "\033[32m"; // Green
            case Level::WARNING:
                return "\033[33m"; // Yellow
            case Level::ERROR:
                return "\033[31m"; // Red
            default:
                return "\033[0m"; // Reset
        }
    }

    static bool ShouldUseColor (Level level) noexcept
    {
#ifdef _MSC_VER
        const int fd = (level == Level::ERROR || level == Level::WARNING) ? _fileno (stderr) : _fileno (stdout);
        return fd >= 0 && _isatty (fd) != 0;
#else
        const int fd = (level == Level::ERROR || level == Level::WARNING) ? STDERR_FILENO : STDOUT_FILENO;
        return isatty (fd) != 0;
#endif
    }

    static inline std::mutex log_mutex;
    static inline Level min_level = Level::INFO; // Default to INFO level in production
};
