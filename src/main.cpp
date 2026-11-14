// webserv.cpp
// ----------------------------------------------------------------------------

#include <exception>
#include <iostream>
#include <vector>
#include "../inc/ConfigParser.hpp"
#include "../inc/ServerCore.hpp"

#define DEFAULT_CONFIG "default.conf"

int main(int argc, char** argv) {
	if (argc > 2) {
		std::cout << "ERROR: too many arguments" << std::endl;
		return 1;
	}
	std::string conf_path = DEFAULT_CONFIG;
	if (argc == 2) {
		conf_path = argv[1];
	}
	ConfigParse configParser(conf_path);
	configParser.fileToken();
	if (configParser.parseManager() == -1)
		return 2;
	std::vector<ServerConfig> servers = configParser.getServers();
	try {
		ServerCore core(servers);
		core.setup();
		core.run();
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
