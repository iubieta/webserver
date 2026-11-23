// log_macros.hpp
//
// Macro-only stream façade over the existing Logger methods.
// Keeps the Logger API untouched: each macro builds the message with an
// ostringstream and forwards it (plus __FILE__/__LINE__) to the global logger.
// ----------------------------------------------------------------------------

#ifndef LOG_MACROS_HPP
#define LOG_MACROS_HPP

#include <sstream>

#include "log_global.hpp"

#define FT_LOG_DETAIL_EMIT(method, message, file, line) \
	do { \
		std::ostringstream _ft_log_stream_; \
		_ft_log_stream_ << message; \
		ft_log::global().method(_ft_log_stream_.str(), (file), (line)); \
	} while (0)

#define LOG_DEBUG(message) \
	FT_LOG_DETAIL_EMIT(debug, message, __FILE__, __LINE__)

#define LOG_INFO(message) \
	FT_LOG_DETAIL_EMIT(info, message, __FILE__, __LINE__)

#define LOG_WARNING(message) \
	FT_LOG_DETAIL_EMIT(warning, message, __FILE__, __LINE__)

#define LOG_ERROR(message) \
	FT_LOG_DETAIL_EMIT(error, message, __FILE__, __LINE__)

#define LOG_CRITICAL(message) \
	FT_LOG_DETAIL_EMIT(critical, message, __FILE__, __LINE__)

#endif // LOG_MACROS_HPP
