#pragma once

#ifndef LOG_SUMMARY
#define LOG_SUMMARY

#include <unordered_map>
#include <string>
#include <vector>
#include <ranges>
#include <algorithm>

#include "log_types.h"


class LogSummary
{
public:
    // Build a LogSummary from a range of LogEntry objects via for_each/accumulation
    template<std::ranges::viewable_range Range>
    static LogSummary from(Range&& entries)
    {
        LogSummary summary;
        std::ranges::for_each(std::forward<Range>(entries), [&summary](const LogEntry& entry) {
            summary.counts[entry.level]++;
            summary.pattern_counts[entry.source]++;
        });
        return summary;
    }

    // Count entries for a specific log level
    size_t count_for(LogLevel level) const
    {
        auto it = counts.find(level);
        return (it != counts.end()) ? it->second : 0;
    }

    // Get total count of all entries
    size_t total() const
    {
        size_t sum = 0;
        for (const auto& [level, count] : counts) {
            sum += count;
        }
        return sum;
    }

    // Get top N patterns (sources with highest counts)
    std::vector<std::pair<std::string, size_t>> top_n(size_t n) const
    {
        std::vector<std::pair<std::string, size_t>> result(pattern_counts.begin(), pattern_counts.end());
        std::ranges::sort(result, [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
        if (result.size() > n) {
            result.resize(n);
        }
        return result;
    }

private:
    std::unordered_map<std::string, size_t> pattern_counts;
    std::unordered_map<LogLevel, size_t> counts;      
};

#endif // !LOG_SUMMARY
