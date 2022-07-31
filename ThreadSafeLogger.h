#ifndef ThreadSafeLogger_LOG_H_
#define ThreadSafeLogger_LOG_H_

#include <memory>
#include "Logger.h"

struct ThreadSafeLoggerImpl;

class ThreadSafeLogger
{
public:
	ThreadSafeLogger(const std::string& log_prefix, const std::string& log_directory);
	virtual ~ThreadSafeLogger();

	void save(LOGGER::internal::LogEntry entry);

	void fatal(LOGGER::internal::FatalMessage fatal_message);

	std::string logFileName() const;

private:
	std::unique_ptr<ThreadSafeLoggerImpl> pimpl_;
	const std::string log_file_with_path_;

	ThreadSafeLogger(const ThreadSafeLogger&); // c++11 feature not yet in vs2010 = delete;
	ThreadSafeLogger& operator=(const ThreadSafeLogger&); // c++11 feature not yet in vs2010 = delete;
};

#endif // ThreadSafeLogger_LOG_H_
