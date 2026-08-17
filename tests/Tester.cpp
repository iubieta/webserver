// Tester.cpp
// ----------------------------------------------------------------------------

#include "Tester.hpp"

static int	failures_ = 0;
static int	testsRun_ = 0;
static int	testsPassed_ = 0;

void	Tester::runTest(const std::string &name, void (*fn)()) {
	int before = failures_;
	fn();
	++testsRun_;
	std::cerr << "Test " << testsRun_ << " : ";
	if (before == failures_) {
		++testsPassed_;
		std::cerr << name << " --> " << GREEN << "OK" << RESET << std::endl;
	}
	else {
		std::cerr << name << " --> " << RED << "KO" << RESET << std::endl;
	}
}

void	Tester::recordFail() {
	++failures_;
}

int		Tester::report() {
	int error = 0;
	if (testsRun_ == 0)
		return 1;
	std::cerr << "==============================" << std::endl;
	if (testsPassed_ < testsRun_) {
		std::cerr << RED << "FAIL: "
			<< testsPassed_ << "/" << testsRun_ 
			<< " tests passed" << std::endl;
		error = 1;
	}
	else 
		std::cerr << GREEN << "OK! All tests passed (" 
			<< testsPassed_ << "/" << testsRun_ 
			<< ")" << std::endl;
	std::cerr << RESET;
	return error;
}

