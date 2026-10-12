#pragma once
#ifndef LOG_PIPELINE
#define LOG_PIPELINE

#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

#include "log_parsing.h"
#include "log_types.h"

template<std::ranges::viewable_range Range>
auto SignificantLines (Range &&lines)
{
    return std::views::all (std::forward<Range> (lines)) | std::views::filter (LooksLikeWarningOrError);
}

template<std::ranges::viewable_range Range>
auto ParsedEntries (Range &&lines)
{
    return std::views::all (std::forward<Range> (lines)) | std::views::transform (ParseLine)
            | std::views::filter ([](const std::optional<LogEntry>& entry) { return entry.has_value (); })
            | std::views::transform ([] (const std::optional<LogEntry>& entry) { return *entry;});
}

template<std::ranges::viewable_range Range>
auto ByLevel (Range &&entries, LogLevel level)
{
    return std::views::all (std::forward<Range> (entries)) 
      | std::views::filter ([level] (const LogEntry &entry) { return entry.level == level;});
}

template<std::ranges::viewable_range Range>
auto WarningOrErrorEntries (Range &&lines)
{
    return ParsedEntries (SignificantLines (std::forward<Range> (lines)));
}

template<std::ranges::viewable_range Range>
auto InTimeRange (Range &&entries, std::string_view lower, std::string_view upper)
{
    const std::string lower_bound {lower};
    const std::string upper_bound {upper};

    return std::views::all (std::forward<Range> (entries))
           | std::views::filter ([lower_bound, upper_bound] (const LogEntry &entry)
                   { return entry.timestamp >= lower_bound && entry.timestamp <= upper_bound; });
}

template<std::ranges::viewable_range Range>
auto ByModule (Range &&entries, std::string_view module)
{
    const std::string module_name {module};

    return std::views::all (std::forward<Range> (entries))
           | std::views::filter ([module_name] (const LogEntry &entry) { return entry.source == module_name; });
}

template<std::ranges::viewable_range Range>
auto Windowed (Range &&entries, std::size_t count, std::size_t skip)
{
    return std::views::all (std::forward<Range> (entries)) | std::views::drop (skip) | std::views::take (count);
}

#endif // !LOG_PIPELINE
