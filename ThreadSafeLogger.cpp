
#include "ThreadSafeLogger.h"

#include <iostream>
#include <functional>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>
#include <iomanip>
#include <ctime>

#include "active.h"
#include "Logger.h"


using namespace LOGGER::internal;

namespace
{
	struct LogTime
	{
		LogTime()
		{
			time_t current_time = time(nullptr);
			ctime(&current_time); // fill with time right now
			struct tm* ptm = ::localtime(&current_time); // fill time struct with data
			year = ptm->tm_year + 1900;
			month = (ptm->tm_mon) + 1;
			day = ptm->tm_mday;
			hour = ptm->tm_hour;
			minute = ptm->tm_min;
			second = ptm->tm_sec;
		}
		int year;   // Year	- 1900
		int month;  // [1-12]
		int day;    // [1-31]
		int hour;   // [0-23]
		int minute; // [0-59]
		int second; // [0-60], 1 leap second
	};

	bool isValidFilename(const std::string prefix_filename)
	{
		std::string illegal_characters("/,|<>:#$%{}()[]\'\"^!?+* ");
		size_t pos = prefix_filename.find_first_of(illegal_characters, 0);
		if (pos != std::string::npos)
		{
			std::cerr << "Illegal character [" << prefix_filename.at(pos) << "] in logname prefix: " << "[" << prefix_filename << "]" << std::endl;
			return false;
		}
		else if (prefix_filename.empty())
		{
			std::cerr << "Empty filename prefix is not allowed" << std::endl;
			return false;
		}
		return true;
	}
}


struct ThreadSafeLoggerImpl
{
	ThreadSafeLoggerImpl(const std::string& log_prefix, const std::string& log_directory);
	~ThreadSafeLoggerImpl();

	void backgroundFileWrite(LOGGER::internal::LogEntry message);
	void backgroundExitFatal(LOGGER::internal::FatalMessage fatal_message);

	std::string log_file_with_path_;
	std::unique_ptr<Utility::Active> bg_;
	std::ofstream out;
	LOGGER::internal::time_point start_time_;

private:
	ThreadSafeLoggerImpl& operator=(const ThreadSafeLoggerImpl&) = delete;
	ThreadSafeLoggerImpl(const ThreadSafeLoggerImpl& other) = delete;
};


ThreadSafeLoggerImpl::ThreadSafeLoggerImpl(const std::string& log_prefix, const std::string& log_directory)
	: log_file_with_path_(log_directory)
	, bg_(Utility::Active::createActive())
	, start_time_(std::chrono::steady_clock::now())
{
	std::string real_prefix = log_prefix;
	std::remove(real_prefix.begin(), real_prefix.end(), '/');
	std::remove(real_prefix.begin(), real_prefix.end(), '\\');
	if (!isValidFilename(real_prefix))
	{
		std::cerr << "g2log: forced abort due to illegal log prefix [" << log_prefix << "]" << std::endl << std::flush;
		abort();
	}

	using namespace std;
	LogTime t;
	ostringstream oss_name;
	oss_name.fill('0');
	oss_name << real_prefix << ".g2log.";
	oss_name << t.year << setw(2) << t.month << setw(2) << t.day;
	oss_name << "-" << setw(2) << t.hour << setw(2) << t.minute << setw(2) << t.second;
	oss_name << ".log";
	log_file_with_path_ += oss_name.str();


	std::ios_base::openmode mode = std::ios_base::out;
	mode |= std::ios_base::trunc;
	out.open(log_file_with_path_, mode);
	if (!out.is_open())
	{
		std::ostringstream ss_error;
		ss_error << "Fatal error could not open log file:[" << log_file_with_path_ << "]";
		ss_error << "\n\t\t std::ios_base state = " << out.rdstate();
		std::cerr << ss_error.str().c_str() << std::endl << std::flush;
		assert(false && "cannot open log file at startup");
	}
	std::ostringstream ss_entry;
	time_t creation_time;
	time(&creation_time);
	ss_entry << "\t\tg2log created log file at: " << ctime(&creation_time);
	ss_entry << "\t\tLOG format: [YYYY/MM/DD hh:mm:ss.uuu* LEVEL FILE:LINE] message\n\n";
	out << ss_entry.str() << std::flush;
	out.fill('0');
}

ThreadSafeLoggerImpl::~ThreadSafeLoggerImpl()
{
	std::ostringstream ss_exit;
	time_t exit_time;
	time(&exit_time);
	bg_.reset();
	ss_exit << "\n\t\tg2log file shutdown at: " << ctime(&exit_time);
	out << ss_exit.str() << std::flush;
}

void ThreadSafeLoggerImpl::backgroundFileWrite(LogEntry message)
{
	using namespace std;
	LogTime t;
	auto timesnapshot = chrono::steady_clock::now();
	out << "\n" << t.year << "/" << setw(2) << t.month << "/" << setw(2) << t.day;
	out << " " << setw(2) << t.hour << ":" << setw(2) << t.minute << ":" << setw(2) << t.second;
	out << "." << chrono::duration_cast<microsecond>(timesnapshot - start_time_).count(); //microseconds
	out << "\t" << message << std::flush;
}

void ThreadSafeLoggerImpl::backgroundExitFatal(FatalMessage fatal_message)
{
	backgroundFileWrite(fatal_message.message_);
	backgroundFileWrite("Log flushed successfully to disk: exiting");
	std::cout << "g2log exiting successfully after receiving fatal event" << std::endl;
	std::cout << "Log file at: [" << log_file_with_path_ << "]\n" << std::endl << std::flush;
	out.close();
	perror("g2log exited after receiving FATAL trigger. Flush message status: ");
}

ThreadSafeLogger::ThreadSafeLogger(const std::string& log_prefix, const std::string& log_directory)
	: pimpl_(new ThreadSafeLoggerImpl(log_prefix, log_directory))
	, log_file_with_path_(pimpl_->log_file_with_path_)
{
}

ThreadSafeLogger::~ThreadSafeLogger()
{
	pimpl_.reset();
	std::cout << "\nExiting, log location: " << log_file_with_path_ << std::endl << std::flush;
}

void ThreadSafeLogger::save(LOGGER::internal::LogEntry msg)
{
	pimpl_->bg_->send(std::bind(&ThreadSafeLoggerImpl::backgroundFileWrite, pimpl_.get(), msg));
}

void ThreadSafeLogger::fatal(LOGGER::internal::FatalMessage fatal_message)
{
	pimpl_->bg_->send(std::bind(&ThreadSafeLoggerImpl::backgroundExitFatal, pimpl_.get(), fatal_message));
}

std::string ThreadSafeLogger::logFileName() const
{
	return log_file_with_path_;
}

