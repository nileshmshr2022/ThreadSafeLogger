
#include "ThreadSafeLogger.h"
#include "Logger.h"
#include <iomanip>


int main()
{
	const std::string path_to_log_file = "./";

	ThreadSafeLogger logger("Log-", path_to_log_file);
	LOGGER::initializeLogging(&logger);

	LOGF(INFO, "Hi log %d", 123);
	LOG(INFO) << "Test SLOG INFO";
	LOG(DEBUG) << "Test SLOG DEBUG";
	LOG(INFO) << "one: " << 1;
	LOG(INFO) << "two: " << 2;
	LOG(INFO) << "one and two: " << 1 << " and " << 2;
	LOG(DEBUG) << "float 2.14: " << 1000 / 2.14f;
}