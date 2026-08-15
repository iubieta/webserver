// Logger.hpp
//
// Logger class header: the main purpose of this class is to provide a log tool
// for this or any other project
// ----------------------------------------------------------------------------

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <string>

# define TIME_FORMAT "%Y-%m-%d %H:%M:%S"

class Logger {

public: 
	enum LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

private:
	mutable std::ofstream	logFile_;
	LogLevel				fileLevel_;
	bool					console_;
	LogLevel				consoleLevel_;
	bool					timestamp_;

	std::string				levelToStr_(LogLevel level) const;
	bool					checkFileLevel_(LogLevel level) const;
	bool					checkConsoleLevel_(LogLevel level) const;
	const std::string		getTimestamp_() const;
	void					log_(LogLevel level, const std::string &message,
								const char *file = NULL, int line=0) const;

	Logger(const Logger& other);
	Logger&				operator=(const Logger& other);

public:
	Logger();
	Logger(const std::string &filepath);
	~Logger();

	void					setFileLevel(LogLevel level);
	void					setConsole(bool enabled);
	void					setConsoleLevel(LogLevel level);
	void					setTimestamp(bool enabled);
	void					debug(const std::string &message,
								const char *file = NULL, int line = 0) const;
	void					info(const std::string &message,
								const char *file = NULL, int line = 0) const;
	void					warning(const std::string &message,
								const char *file = NULL, int line = 0) const;
	void					error(const std::string &message,
								const char *file = NULL, int line = 0) const;
	void					critical(const std::string &message,
								const char *file = NULL, int line = 0) const;
};

#endif // !LOGGER_HPP
