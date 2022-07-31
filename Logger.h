
#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <sstream>
#include <iostream>
#include <cstdarg>
#include <chrono>

class ThreadSafeLogger;

#if !(defined(__PRETTY_FUNCTION__))
#define __PRETTY_FUNCTION__   __FUNCTION__
#endif


const int DEBUG = 0, INFO = 1, WARNING = 2, FATAL = 3;
static const std::string k_fatal_log_expression = "";

#define LOGGER_LOG_DEBUG  LOGGER::internal::LogMessage(__FILE__,__LINE__,__PRETTY_FUNCTION__,"DEBUG")
#define LOGGER_LOG_INFO  LOGGER::internal::LogMessage(__FILE__,__LINE__,__PRETTY_FUNCTION__,"INFO")
#define LOGGER_LOG_WARNING  LOGGER::internal::LogMessage(__FILE__,__LINE__,__PRETTY_FUNCTION__,"WARNING")
#define LOGGER_LOG_FATAL  LOGGER::internal::LogContractMessage(__FILE__,__LINE__,__PRETTY_FUNCTION__,k_fatal_log_expression)

#define LOG(level) LOGGER_LOG_##level.messageStream()

#define LOG_IF(level, boolean_expression)  \
  if(true == boolean_expression)          \
     LOGGER_LOG_##level.messageStream()

#define CHECK(boolean_expression)                                                    \
if (false == (boolean_expression))                                                     \
  LOGGER::internal::LogContractMessage(__FILE__, __LINE__, __PRETTY_FUNCTION__, #boolean_expression).messageStream()

#define LOGGER_LOGF_INFO     LOGGER::internal::LogMessage(__FILE__, __LINE__, __PRETTY_FUNCTION__,"INFO")
#define LOGGER_LOGF_DEBUG    LOGGER::internal::LogMessage(__FILE__, __LINE__, __PRETTY_FUNCTION__,"DEBUG")
#define LOGGER_LOGF_WARNING  LOGGER::internal::LogMessage(__FILE__, __LINE__, __PRETTY_FUNCTION__,"WARNING")
#define LOGGER_LOGF_FATAL    LOGGER::internal::LogContractMessage(__FILE__, __LINE__, __PRETTY_FUNCTION__,k_fatal_log_expression)

#define LOGF(level, printf_like_message, ...)                 \
  LOGGER_LOGF_##level.messageSave(printf_like_message, __VA_ARGS__);

#define LOGF_IF(level,boolean_expression, printf_like_message, ...) \
  if(true == boolean_expression)                                     \
     LOGGER_LOG_##level.messageSave(printf_like_message, __VA_ARGS__);

#define CHECK_F(boolean_expression, printf_like_message, ...)                                     \
   if (false == (boolean_expression))                                                             \
  LOGGER::internal::LogContractMessage(__FILE__, __LINE__, __PRETTY_FUNCTION__,#boolean_expression).messageSave(printf_like_message, __VA_ARGS__);

namespace LOGGER
{

	void initializeLogging(ThreadSafeLogger* logger);

	ThreadSafeLogger* shutDownLogging();

	namespace internal
	{
		typedef std::chrono::steady_clock::time_point time_point;
		typedef std::chrono::duration<long, std::ratio<1, 1000> > millisecond;
		typedef std::chrono::duration<long long, std::ratio<1, 1000000> > microsecond;
		typedef std::chrono::duration<long long, std::ratio<1, 1000000> > microsecond;

		typedef const std::string& LogEntry;

		void changeFatalInitHandlerForUnitTesting();

		struct FatalMessage
		{
			enum FatalType { kReasonFatal, kReasonOS_FATAL_SIGNAL };
			FatalMessage(std::string message, FatalType type, int signal_id);

			std::string message_;
			FatalType type_;
			int signal_id_;
		};

		struct FatalTrigger
		{
			FatalTrigger(const FatalMessage& message);
			~FatalTrigger();
			FatalMessage message_;
		};

		class LogMessage
		{
		public:
			LogMessage(const std::string& file, const int line, const std::string& function_, const std::string& level);
			virtual ~LogMessage();

			std::ostringstream& messageStream() { return stream_; }
#ifndef __GNUC__
#define  __attribute__(x)
#endif		
			void messageSave(const char* printf_like_message, ...)
				__attribute__((format(printf, 2, 3)));

		protected:
			const std::string file_;
			const int line_;
			const std::string function_;
			const std::string level_;
			std::ostringstream stream_;
			std::string log_entry_;
		};

		class LogContractMessage : public LogMessage
		{
		public:
			LogContractMessage(const std::string& file, const int line,
				const std::string& function, const std::string& boolean_expression);
			virtual ~LogContractMessage(); // at destruction will flush the message

		protected:
			const std::string expression_;
		};
	} // end namespace internal
} // end namespace LOGGER

#endif
