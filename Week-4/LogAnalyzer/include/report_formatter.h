#pragma once
#ifndef REPORT_FORMATTER
#define REPORT_FORMATTER

#include <string>
#include <ostream>
#include <ranges>
#include <iomanip>

#include "log_summary.h"
#include "log_types.h"

/*
ReportFormatter (class — presentation only)
    + format(const LogSummary&) -> string
    + print(const LogSummary&, ostream&)
*/

class ReportFormatter
{
public:
    // Format a LogSummary as a formatted string
    static std::string Format(const LogSummary& summary)
    {
        std::string report;

        report += "=================================================\n";
        report += "             LOG ANALYSIS REPORT                 \n";
        report += "=================================================\n\n";

        // Total count
        report += "OVERALL STATISTICS:\n";
        report += "-------------------\n";
        report += std::string("Total Entries: ") + std::to_string(summary.total()) + "\n\n";

        // Breakdown by log level
        report += "BREAKDOWN BY LOG LEVEL:\n";
        report += "----------------------\n";

        constexpr LogLevel levels[] = {LogLevel::DEBUG, LogLevel::INFO, LogLevel::WARNING, LogLevel::ERROR};
        size_t maxCount = 0;
        for (const auto& level : levels) {
            maxCount = std::max(maxCount, summary.count_for(level));
        }

        for (const auto& level : levels) {
            const auto count = summary.count_for(level);
            const auto levelStr = ToString(level);
            report += std::string("  ") + std::string(levelStr) + ": " + std::to_string(count);

            // Add a simple bar chart
            if (maxCount > 0) {
                const auto barLength = (count * 30) / maxCount;
                report += " [";
                for (size_t i = 0; i < barLength; ++i) {
                    report += "=";
                }
                report += "]\n";
            } else {
                report += "\n";
            }
        }

        report += "\n";

        // Top patterns (sources)
        const auto topPatterns = summary.top_n(5);
        if (!topPatterns.empty()) {
            report += "TOP 5 LOG SOURCES:\n";
            report += "-------------------\n";
            size_t rank = 1;
            for (const auto& [source, count] : topPatterns) {
                report += std::string("  ") + std::to_string(rank) + ". " + source + 
                         " (" + std::to_string(count) + " entries)\n";
                rank++;
            }
        }

        report += "\n=================================================\n";

        return report;
    }

    // Print a LogSummary to an ostream
    static void print(const LogSummary& summary, std::ostream& os)
    {
        os << Format(summary);
    }
};

#endif // !REPORT_FORMATTER
