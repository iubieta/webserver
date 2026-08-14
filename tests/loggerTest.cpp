// test_logger.cpp
//
// Basic testing file for Logger class
// ----------------------------------------------------------------------------

#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <cstdio>
#include <strings.h>

#include "Tester.hpp"
#include "../inc/Logger.hpp"

// Auxiliar methods
// ----------------------------------------------------------------------------

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

std::streambuf	*redirCerr(std::ostringstream &out) {
	std::streambuf *ogStream = std::cerr.rdbuf();
	std::streambuf *newStream = out.rdbuf();
	std::cerr.rdbuf(newStream);
	return ogStream;
}

void	resetCerr(std::streambuf *ogStream) {
	std::cerr.rdbuf(ogStream);
}

int		countCharInStr(char c, const std::string str) {
	size_t i = 0;
	int count = 0;
	i = str.find_first_of(c, i);
	while (i != std::string::npos) {
		++count;
		i = str.find_first_of(c, ++i);
	}
	return count;
}

// Test 1: File opening error
// ----------------------------------------------------------------------------
void	testFileError() {
	std::string	logfile = "/unknown/path/to/file.log";

	std::ostringstream consoleOut;
	std::streambuf *ogStream = redirCerr(consoleOut);
	Logger	log(logfile);
	resetCerr(ogStream);

	std::string out = consoleOut.str();
	std::string expected = "[WARNING] Log file could not be opened, continuing in console mode\n";
	ASSERT_EQ(out, expected);
}

// Test 2: File Level Filtering
// ----------------------------------------------------------------------------
void	testFileLevel(Logger::LogLevel level, int expected) {
	std::string	logfile = "temp.log";
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


void	testFileLevelFiltering() {
	testFileLevel(Logger::DEBUG, 5);
	testFileLevel(Logger::INFO, 4);
	testFileLevel(Logger::WARNING, 3);
	testFileLevel(Logger::ERROR, 2);
	testFileLevel(Logger::CRITICAL, 1);
}

// Test 3: Console Level Filtering
// ----------------------------------------------------------------------------
void	testConsoleLevel(Logger::LogLevel level, int expected) {
	
	std::ostringstream consoleOut;
	std::streambuf *ogStream = redirCerr(consoleOut);
	Logger	log;
	log.setConsoleLevel(level);
	logEveryLevel(log);
	resetCerr(ogStream);
	int lines = 0;
	lines = countCharInStr('\n', consoleOut.str());
	ASSERT_EQ(lines, expected);
}

void	testConsoleLevelFiltering() {
	testConsoleLevel(Logger::DEBUG, 5);
	testConsoleLevel(Logger::INFO, 4);
	testConsoleLevel(Logger::WARNING, 3);
	testConsoleLevel(Logger::ERROR, 2);
	testConsoleLevel(Logger::CRITICAL, 1);
}

// Test battery
// ----------------------------------------------------------------------------
int main() {
	Tester::runTest("File error", testFileError);
	Tester::runTest("File Level filtering", testFileLevelFiltering);
	Tester::runTest("Console Level filtering", testConsoleLevelFiltering);
	return Tester::report();
}
