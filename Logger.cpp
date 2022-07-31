
#include "Logger.h"

#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept> // exceptions
#include <cstdio>    // vsnprintf
#include <cassert>
#include <mutex>

#include "ThreadSafeLogger.h"

#include <signal.h>
#include <thread>

namespace LOGGER
{
	namespace constants
	{
		const int kMaxMessageSize = 2048;
		const std::string kTruncatedWarningText = "[...truncated...]";
	}
	namespace internal
	{
		static ThreadSafeLogger* g_logger_instance = nullptr; // instantiated and OWNED somewhere else (main)
		static std::mutex g_logging_init_mutex;
		bool isLoggingInitialized() { return g_logger_instance != nullptr; }

		std::string splitFileName(const std::string& str)
		{
			size_t found;
			found = str.find_last_of("(/\\");
			return str.substr(found + 1);
		}

	}


	void initializeLogging(ThreadSafeLogger* bgworker)
	{
		static bool once_only_signalhandler = false;
		std::lock_guard<std::mutex> lock(internal::g_logging_init_mutex);
		CHECK(!internal::isLoggingInitialized());
		CHECK(bgworker != nullptr);
		internal::g_logger_instance = bgworker;

		if (false == once_only_signalhandler)
		{
			once_only_signalhandler = true;
		}
	}

	ThreadSafeLogger* shutDownLogging()
	{
		std::lock_guard<std::mutex> lock(internal::g_logging_init_mutex);
		CHECK(internal::isLoggingInitialized());
		ThreadSafeLogger* backup = internal::g_logger_instance;
		internal::g_logger_instance = nullptr;
		return backup;
	}



	namespace internal
	{

		void callFatalInitial(FatalMessage message)
		{
			internal::g_logger_instance->fatal(message);
		}

		void (*g_fatal_to_ThreadSafeLogger_function_ptr)(FatalMessage) = callFatalInitial;


		void unitTestFatalInitHandler(LOGGER::internal::FatalMessage fatal_message)
		{
			assert(internal::g_logger_instance != nullptr);
			internal::g_logger_instance->save(fatal_message.message_);
			throw std::runtime_error(fatal_message.message_);
		}

		void changeFatalInitHandlerForUnitTesting()
		{
			g_fatal_to_ThreadSafeLogger_function_ptr = unitTestFatalInitHandler;
		}





		LogContractMessage::LogContractMessage(const std::string& file, const int line,
			const std::string& function, const std::string& boolean_expression)
			: LogMessage(file, line, function, "FATAL")
			, expression_(boolean_expression)
		{}

		LogContractMessage::~LogContractMessage()
		{
			std::ostringstream oss;
			if (0 == expression_.compare(k_fatal_log_expression))
			{
				oss << "\n[  *******\tEXIT trigger caused by LOG(FATAL): \n\t";
			}
			else
			{
				oss << "\n[  *******\tEXIT trigger caused by broken Contract: CHECK(" << expression_ << ")\n\t";
			}
			log_entry_ = oss.str();
		}

		LogMessage::LogMessage(const std::string& file, const int line, const std::string& function, const std::string& level)
			: file_(file)
			, line_(line)
			, function_(function)
			, level_(level)

		{}


		LogMessage::~LogMessage()
		{
			using namespace internal;
			std::ostringstream oss;
			const bool fatal = (0 == level_.compare("FATAL"));
			oss << level_ << " [" << splitFileName(file_);
			if (fatal)
				oss << " at: " << function_;
			oss << " L: " << line_ << "]\t";

			const std::string str(stream_.str());
			if (!str.empty())
			{
				oss << '"' << str << '"';
			}
			log_entry_ += oss.str();

			if (!isLoggingInitialized())
			{
				std::cerr << "Did you forget to call LOGGER::InitializeLogging(ThreadSafeLogger*) in your main.cpp?" << std::endl;
				std::cerr << log_entry_ << std::endl << std::flush;
				throw std::runtime_error("Logger not initialized with LOGGER::InitializeLogging(ThreadSafeLogger*) for msg:\n" + log_entry_);
			}


			if (fatal)
			{
				{
					FatalMessage::FatalType fatal_type(FatalMessage::kReasonFatal);
					FatalMessage fatal_message(log_entry_, fatal_type, SIGABRT);
					FatalTrigger trigger(fatal_message);
					std::cerr << log_entry_ << "\t*******  ]" << std::endl << std::flush;
				}
			}
			internal::g_logger_instance->save(log_entry_);
		}


		FatalMessage::FatalMessage(std::string message, FatalType type, int signal_id)
			: message_(message)
			, type_(type)
			, signal_id_(signal_id) {}

		FatalTrigger::FatalTrigger(const FatalMessage& message)
			: message_(message) {}

		FatalTrigger::~FatalTrigger()
		{
			g_fatal_to_ThreadSafeLogger_function_ptr(message_);
			while (true) { std::this_thread::sleep_for(std::chrono::seconds(1)); }
		}



		void LogMessage::messageSave(const char* printf_like_message, ...)
		{
			char finished_message[constants::kMaxMessageSize];
			va_list arglist;
			va_start(arglist, printf_like_message);
			const int nbrcharacters = vsnprintf(finished_message, sizeof(finished_message), printf_like_message, arglist);
			va_end(arglist);
			if (nbrcharacters <= 0)
			{
				stream_ << "\n\tERROR LOG MSG NOTIFICATION: Failure to parse successfully the message";
				stream_ << '"' << printf_like_message << '"' << std::endl;
			}
			else if (nbrcharacters > constants::kMaxMessageSize)
			{
				stream_ << finished_message << constants::kTruncatedWarningText;
			}
			else
			{
				stream_ << finished_message;
			}
		}

	} // end of namespace LOGGER::internal
} // end of namespace LOGGER
