
#include "log_types.h"
#include "logger.h"

int main ()
{ 
	// Initialize logger from environment variables
	Logger::InitializeFromEnvironment ();
    Logger::Log (Logger::Level::INFO, "Application started.");

	return 0;
}
