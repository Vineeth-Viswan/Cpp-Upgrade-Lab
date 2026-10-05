#pragma once
#ifndef LOG_PARSER
#define LOG_PARSER

#include <optional>
#include <string_view>
#include <string>

#include "log_types.h"

std::optional<LogEntry> ParseLine (const RawLine &line)
{
    constexpr std::string_view delimiter = " | ";

    const auto timestamp_end = line.content.find (delimiter);
    if (timestamp_end == std::string::npos || timestamp_end == 0)
    {
        return std::nullopt;
    }

    const auto level_start = timestamp_end + delimiter.size ();
    const auto level_end = line.content.find (delimiter, level_start);
    if (level_end == std::string::npos || level_end == level_start)
    {
        return std::nullopt;
    }

    const auto level = FromString (std::string_view{line.content}.substr (level_start, level_end - level_start));
    if (!level)
    {
        return std::nullopt;
    }

    const auto source_start = level_end + delimiter.size ();
    const auto source_end = line.content.find (delimiter, source_start);
    if (source_end == std::string::npos || source_end == source_start)
    {
        return std::nullopt;
    }

    const auto message_start = source_end + delimiter.size ();

    return LogEntry{
        *level,
        line.content.substr (0, timestamp_end),
        line.content.substr (source_start, source_end - source_start),
        line.content.substr (message_start),
        line.line_number
    };
}

bool LooksLikeWarningOrError (const RawLine &line)
{
    constexpr std::string_view delimiter = " | ";
    const auto timestamp_end = line.content.find (delimiter);
    if (timestamp_end == std::string::npos)
    {
        return false;
    }

    const auto level_start = timestamp_end + delimiter.size ();
    const auto level_end = line.content.find (delimiter, level_start);
    if (level_end == std::string::npos)
    {
        return false;
    }

    const auto level = std::string_view{line.content}.substr (
        level_start, level_end - level_start);
    return level == "WARNING" || level == "ERROR";
}

#endif // !LOG_PARSER
