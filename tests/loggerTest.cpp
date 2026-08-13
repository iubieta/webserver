// test_logger.cpp
//
// Basic testing file for Logger class
// ----------------------------------------------------------------------------

#include <fstream>
#include <string>
#include <cstdio>

#include "Tester.hpp"
#include "../inc/Logger.hpp"

void	logEveryLevel(Logger &log) {
	log.debug("Debug message");
	log.info("Info message");
	log.warning("Warning message");
	log.error("Error message");
	log.critical("Critical message");
}

int		numberOfLines(const std::string &filepath) {
	std::ifstream	file;

	file.open(filepath.c_str());
	if (!file.is_open())
		return -1;
	int count = 0;
	std::string line;
	while (std::getline(file, line)) {
		++count;
	}
	return count;
}

void	checkFilter(Logger::LogLevel level, int expected) {
	std::string	logfile = "levelFiltering.log";
	std::remove(logfile.c_str());
	int lines;
	{
		Logger		log(logfile);
		log.setConsole(0);
		log.setTimestamp(0);
		log.setFileLevel(level);
		logEveryLevel(log);
	}
	lines = numberOfLines(logfile);
	ASSERT_EQ(lines, expected);
	std::remove(logfile.c_str());
}

void	testLevelFiltering() {
	checkFilter(Logger::DEBUG, 5);
	checkFilter(Logger::INFO, 4);
	checkFilter(Logger::WARNING, 3);
	checkFilter(Logger::ERROR, 2);
	checkFilter(Logger::CRITICAL, 1);
}

int main() {
	Tester::runTest("Level filtering", testLevelFiltering);
	return Tester::report();
}
