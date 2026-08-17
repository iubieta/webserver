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
	enum Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };

private:
	mutable std::ofstream	log_file_;
	Level					file_level_;
	bool					console_;
	Level					console_level_;
	bool					timestamp_;

	std::string				levelToStr(Level level) const;
	bool					checkFileLevel(Level level) const;
	bool					checkConsoleLevel(Level level) const;
	const std::string		getTimestamp() const;
	void					log(Level level, const std::string &message,
								const char *file = NULL, int line=0) const;

	Logger(const Logger& other);
	Logger&				operator=(const Logger& other);

public:
	Logger();
	Logger(const std::string &filepath);
	~Logger();

	void					setFileLevel(Level level);
	void					setConsole(bool enabled);
	void					setConsoleLevel(Level level);
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
