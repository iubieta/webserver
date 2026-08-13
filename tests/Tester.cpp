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
	if (before == failures_) {
		++testsPassed_;
		std::cerr << GREEN << name << "OK" << WHITE << std::endl;
	}
	else {
		std::cerr << RED << name << " KO" << WHITE << std::endl;
	}
}

void	Tester::recordFail() {
	++failures_;
}

int		Tester::report() {
	int error = 0;
	if (testsRun_ == 0)
		return 1;
	if (testsPassed_ < testsRun_) {
		std::cerr << RED << "FAIL: "
			<< testsPassed_ << "/" << testsRun_ 
			<< " tests passed" << std::endl;
		error = 1;
	}
	else 
		std::cerr << GREEN << " OK! All tests passed (" 
			<< testsPassed_ << "/" << testsRun_ 
			<< ")" << std::endl;
	std::cerr << WHITE;
	return error;
}

