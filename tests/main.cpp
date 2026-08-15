// Testing main.cpp
// ----------------------------------------------------------------------------

#include <cstring>
#include <iostream>

#include "TestList.hpp"

struct	TestEntry {
	const char	*name;
	void		(*fn)();
};

static const TestEntry g_tests[] = {
	{ "logger" , logger_tests },
};

static size_t g_count = sizeof(g_tests) / sizeof(TestEntry);

bool	should_run_test(const char *name, int argc, char **argv) {
	if (argc <= 1)
		return true;
	for (int i = 0; i < argc; ++i) {
		if (std::strcmp(name, argv[i]) == 0)
			return true;
	}
	return false;
}

int main(int argc, char **argv) {
	for (size_t i = 0; i < g_count; ++i) {
		if (!should_run_test(g_tests[i].name, argc, argv))
			continue;
		std::cout << "=== " << g_tests[i].name << " ===" << std::endl;
		g_tests[i].fn();
	}
	return 0;
}
