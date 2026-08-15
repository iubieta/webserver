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

bool	fileHasLine(std::string &path, std::string str) {
	std::ifstream	file;
	file.open(path.c_str(), std::ifstream::in);
	if (!file.is_open())
		return false;
	std::string		line;
	while (std::getline(file, line)) {
		if (line == str)
			return true;
	}
	return false;
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


// Test 4: Console and file level independance
// ----------------------------------------------------------------------------
void	checkLevelIndependence(
	Logger::LogLevel fileLvl, int FileExpec,
	Logger::LogLevel consoleLvl, int ConsoleExpec) 
{
	std::string	logfile = "temp.log";
	std::remove(logfile.c_str());
	std::ostringstream consoleOut;
	std::streambuf *ogStream = redirCerr(consoleOut);
	Logger	log(logfile);
	log.setFileLevel(fileLvl);
	log.setConsoleLevel(consoleLvl);
	logEveryLevel(log);
	resetCerr(ogStream);
	int fileLines = numberOfLines(logfile);
	int consoleLines = countCharInStr('\n', consoleOut.str());
	ASSERT_EQ(fileLines, FileExpec);
	ASSERT_EQ(consoleLines, ConsoleExpec);
	std::remove(logfile.c_str());
}

void	testLevelIndependance() {
	checkLevelIndependence(Logger::DEBUG, 5, Logger::INFO, 4);
	checkLevelIndependence(Logger::WARNING, 3, Logger::CRITICAL, 1);
}

// Test 5: Console off
// ----------------------------------------------------------------------------
void	testConsoleOff() {
	std::ostringstream consoleOut;
	std::streambuf *ogStream = redirCerr(consoleOut);
	Logger	log;
	log.setConsoleLevel(Logger::DEBUG);
	logEveryLevel(log);
	log.setConsole(0);
	logEveryLevel(log);
	resetCerr(ogStream);
	int consoleLines = countCharInStr('\n', consoleOut.str());
	ASSERT_EQ(consoleLines, 5);
}

// Test 6: Append mode
// ----------------------------------------------------------------------------

void	testAppendMode() {
	std::string	logfile = "temp.log";
	std::remove(logfile.c_str());
	
	int lines;
	{
		Logger		log(logfile);
		log.setConsole(0);
		log.setTimestamp(0);
		logEveryLevel(log);
	}
	lines = numberOfLines(logfile);
	ASSERT_EQ(lines, 5);
	
	{
		Logger		log(logfile);
		log.setConsole(0);
		log.setTimestamp(0);
		logEveryLevel(log);
	}
	lines = numberOfLines(logfile);
	ASSERT_EQ(lines, 10);
	std::remove(logfile.c_str());
}

// Test 7: Message Fidelity
// ----------------------------------------------------------------------------
void	testMessageFidelity() {
	std::string	logfile = "temp.log";
	std::remove(logfile.c_str());
	std::ostringstream consoleOut;
	std::streambuf *ogStream = redirCerr(consoleOut);
	Logger	log(logfile);
	log.setTimestamp(0);
	log.info("Mensaje de info", __FILE__, 100);
	resetCerr(ogStream);
	std::string fileExpec = "[INFO] Mensaje de info - ";
	fileExpec += std::string(__FILE__) + ":100";
	std::string consoleExpec = "[INFO] Mensaje de info\n";
	ASSERT_EQ(fileHasLine(logfile, fileExpec), true);
	ASSERT_EQ(consoleOut.str(), consoleExpec);
	std::remove(logfile.c_str());
}
// Test battery
// ----------------------------------------------------------------------------
int main() {
	Tester::runTest("File error", testFileError);
	Tester::runTest("File Level filtering", testFileLevelFiltering);
	Tester::runTest("Console Level filtering", testConsoleLevelFiltering);
	Tester::runTest("Level Independence", testLevelIndependance);
	Tester::runTest("Console Off", testConsoleOff);
	Tester::runTest("Append Mode", testAppendMode);
	Tester::runTest("Message Fidelity", testMessageFidelity);
	return Tester::report();
}
