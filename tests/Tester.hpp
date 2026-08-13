// Tester.hpp
// ----------------------------------------------------------------------------

#ifndef TESTER_HPP
#define TESTER_HPP

#include <string>
#include <iostream>

#define BLACK   "\x1B[30m"
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define WHITE   "\x1B[37m"

#define ASSERT_EQ(actual, expected)							\
    do {                                                	\
        if ((actual) != (expected)) {                   	\
			std::cerr										\
				<< __FILE__ << ":" << __LINE__ << " - "		\
				<< "ASSERT_EQ(" << #actual << #expected		\
				<< ") Failed."								\
				<< " Expected: " << expected				\
				<< ", Received: " << actual					\
				<< std::endl;								\
			Tester::recordFail();							\
            return;                                         \
        }                                                   \
    } while (0)

namespace Tester {
	void	runTest(const std::string &name, void (*fn)());
	void	recordFail();
	int		report();
}

#endif // !TESTER_HPP

