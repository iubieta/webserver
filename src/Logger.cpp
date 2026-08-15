// Logger.cpp
//
// Logger class implementation.
// ----------------------------------------------------------------------------

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <ctime>
#include <cstdlib>

#include "../inc/Logger.hpp"

// Canonichal form ------------------------------------------------------------
Logger::Logger() : 
	fileLevel_(DEBUG),
	console_(true), consoleLevel_(INFO),
	timestamp_(true)
{}

Logger::Logger(const std::string &filepath) : 
	fileLevel_(DEBUG), 
	console_(true), consoleLevel_(INFO),
	timestamp_(true)
{
	logFile_.open(filepath.c_str(), std::ios::app);
	if (!logFile_.is_open()) {
		log_(WARNING, "Log file could not be opened, continuing in console mode");
	}
}

Logger::~Logger() {}

// Private methods ------------------------------------------------------------

std::string	Logger::levelToStr_(LogLevel level) const {
	switch (level) {
		case DEBUG:
			return "[DEBUG]";
		case INFO:
			return "[INFO]";
		case WARNING:
			return "[WARNING]";
		case ERROR:
			return "[ERROR]";
		case CRITICAL:
			return "[CRITICAL]";
	}
	return "[UNKNOWN]";
}

bool	Logger::checkFileLevel_(LogLevel level) const {
	return level >= fileLevel_;
}

bool	Logger::checkConsoleLevel_(LogLevel level) const {
	return level >= consoleLevel_;
}

const std::string	Logger::getTimestamp_() const {
	time_t			rawtime = time(NULL);
	struct tm		*timeinfo;
	char			timestr[40];
	
	timeinfo = localtime(&rawtime);
	strftime(timestr, 40, TIME_FORMAT, timeinfo);
	std::string timestamp = "[" + std::string(timestr) + "]";
	return timestamp;
}

void	Logger::log_(LogLevel level, const std::string &message, 
					const char *file, int line) const 
{
	std::string timestr = getTimestamp_();
	std::string levelstr = levelToStr_(level);
	std::string	logstr = levelstr + " " + message;
	if (logFile_.is_open() && checkFileLevel_(level)) {
		if (timestamp_)
			logstr = timestr + " " + logstr; 
		logFile_ << logstr;
		if (file)
			logFile_ << " - " << file << ":" << line;
		logFile_ << std::endl;
	}
	if (console_ && checkConsoleLevel_(level)) {
		std::cerr << logstr << std::endl;;
	}
}

// Setters --------------------------------------------------------------------

void	Logger::setFileLevel(LogLevel level) {
	fileLevel_ = level;
}

void	Logger::setConsole(bool enabled) {
	console_ = enabled;
}

void	Logger::setConsoleLevel(LogLevel level) {
	consoleLevel_ = level;
}

void	Logger::setTimestamp(bool enabled) {
	timestamp_ = enabled;
}

// Logging methods ------------------------------------------------------------
void	Logger::debug(const std::string &message, 
			const char *file, int	line) const {
	log_(DEBUG, message, file, line);
}

void	Logger::info(const std::string &message,
			const char *file, int	line) const {
	log_(INFO, message, file, line);
}

void	Logger::warning(const std::string &message,
			const char *file, int	line) const {
	log_(WARNING, message, file, line);
}

void	Logger::error(const std::string &message,
			const char *file, int	line) const {
	log_(ERROR, message, file, line);
}

void	Logger::critical(const std::string &message,
			const char *file, int	line) const {
	log_(CRITICAL, message, file, line);
}

// int main() {
// 	Logger log("webserv.log");
// 	
// 	log.debug("Hola, esto solo deberia aparecer en el archivo");
// 	log.info("Hola, esto deberia imprimirse por pantalla");
// 	
// 	Logger log2("/log/inexistente.txt");
// }
