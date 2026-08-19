// log_global.cpp
// Logger global instance definition
// ----------------------------------------------------------------------------

#include "../inc/Logger.hpp"

namespace log {
	Logger& global() {
		static Logger instance;
		return instance;
	}
}
