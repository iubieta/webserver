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
	file_level_(DEBUG),
	console_(true), console_level_(INFO),
	timestamp_(true)
{}

Logger::Logger(const std::string &filepath) : 
	file_level_(DEBUG), 
	console_(true), console_level_(INFO),
	timestamp_(true)
{
	log_file_.open(filepath.c_str(), std::ios::app);
	if (!log_file_.is_open()) {
		log(WARNING, "Log file could not be opened, continuing in console mode");
	}
}

Logger::~Logger() {}

// Private methods ------------------------------------------------------------

std::string	Logger::levelToStr(Level level) const {
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

bool	Logger::checkFileLevel(Level level) const {
	return level >= file_level_;
}

bool	Logger::checkConsoleLevel(Level level) const {
	return level >= console_level_;
}

const std::string	Logger::getTimestamp() const {
	time_t			rawtime = time(NULL);
	struct tm		*timeinfo;
	char			timestr[40];
	
	timeinfo = localtime(&rawtime);
	strftime(timestr, 40, TIME_FORMAT, timeinfo);
	std::string timestamp = "[" + std::string(timestr) + "]";
	return timestamp;
}

void	Logger::log(Level level, const std::string &message, 
					const char *file, int line) const 
{
	std::string timestr = getTimestamp();
	std::string levelstr = levelToStr(level);
	std::string	logstr = levelstr + " " + message;
	if (log_file_.is_open() && checkFileLevel(level)) {
		if (timestamp_)
			logstr = timestr + " " + logstr; 
		log_file_ << logstr;
		if (file)
			log_file_ << " - " << file << ":" << line;
		log_file_ << std::endl;
	}
	if (console_ && checkConsoleLevel(level)) {
		std::cerr << logstr << std::endl;;
	}
}

// Setters --------------------------------------------------------------------

void	Logger::setFileLevel(Level level) {
	file_level_ = level;
}

void	Logger::setConsole(bool enabled) {
	console_ = enabled;
}

void	Logger::setConsoleLevel(Level level) {
	console_level_ = level;
}

void	Logger::setTimestamp(bool enabled) {
	timestamp_ = enabled;
}

// Logging methods ------------------------------------------------------------
void	Logger::debug(const std::string &message, 
			const char *file, int	line) const {
	log(DEBUG, message, file, line);
}

void	Logger::info(const std::string &message,
			const char *file, int	line) const {
	log(INFO, message, file, line);
}

void	Logger::warning(const std::string &message,
			const char *file, int	line) const {
	log(WARNING, message, file, line);
}

void	Logger::error(const std::string &message,
			const char *file, int	line) const {
	log(ERROR, message, file, line);
}

void	Logger::critical(const std::string &message,
			const char *file, int	line) const {
	log(CRITICAL, message, file, line);
}

// int main() {
// 	Logger log("webserv.log");
// 	
// 	log.debug("Hola, esto solo deberia aparecer en el archivo");
// 	log.info("Hola, esto deberia imprimirse por pantalla");
// 	
// 	Logger log2("/log/inexistente.txt");
// }
