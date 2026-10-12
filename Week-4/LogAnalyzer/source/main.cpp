
#include <iostream>
#include <string>

#include "log_types.h"
#include "log_loading.h"
#include "log_pipeline.h"
#include "log_summary.h"
#include "report_formatter.h"
#include "logger.h"

int main()
{
	 // Initialize logger from environment variables
	 Logger::InitializeFromEnvironment();
	 Logger::Log(Logger::Level::INFO, "Application started.");

	 try
	 {
		  // Log file path - using example path (change as needed for testing)
		  const std::string logFilePath = "sample.log";

		  Logger::Log(Logger::Level::INFO, "Loading log file: {}", logFilePath);

		  // Step 1: Load the log file
		  LogSource source(logFilePath);
		  Logger::Log(Logger::Level::DEBUG, "Loaded {} lines from log file", source.GetLines().size());

		  // Step 2: Parse the log entries using pipeline
		  auto entries = WarningOrErrorEntries(source.GetLines());
		  Logger::Log(Logger::Level::DEBUG, "Parsing log entries via pipeline");

		  // Step 3: Build a summary from the parsed entries
		  auto summary = LogSummary::from(entries);
		  Logger::Log(Logger::Level::INFO, "Summary built with {} total entries", summary.total());

		  // Step 4: Display statistics
		  Logger::Log(Logger::Level::INFO, "Log Analysis Results:");
		  Logger::Log(Logger::Level::INFO, "  DEBUG entries: {}", summary.count_for(LogLevel::DEBUG));
		  Logger::Log(Logger::Level::INFO, "  INFO entries: {}", summary.count_for(LogLevel::INFO));
		  Logger::Log(Logger::Level::INFO, "  WARNING entries: {}", summary.count_for(LogLevel::WARNING));
		  Logger::Log(Logger::Level::INFO, "  ERROR entries: {}", summary.count_for(LogLevel::ERROR));

		  // Step 5: Format and print the report
		  Logger::Log(Logger::Level::INFO, "Generating formatted report");
		  ReportFormatter::print(summary, std::cout);

		  Logger::Log(Logger::Level::INFO, "Application completed successfully.");
	 }
	 catch (const std::runtime_error& e)
	 {
		  Logger::Log(Logger::Level::ERROR, "Runtime error: {}", e.what());
		  return 1;
	 }
	 catch (const std::exception& e)
	 {
		  Logger::Log(Logger::Level::ERROR, "Unexpected error: {}", e.what());
		  return 2;
	 }

	 return 0;
}
