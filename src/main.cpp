// webserv.cpp
// ----------------------------------------------------------------------------

#include <exception>
#include <iostream>
#include <vector>
#include "../inc/ConfigParser.hpp"
#include "../inc/ServerCore.hpp"
#include "../inc/log_macros.hpp"

#define DEFAULT_CONFIG "config/default.conf"

int main(int argc, char** argv) 
{
	LOG_INFO("========= WEBSERVER STARTED ==========");
	std::string conf_path = DEFAULT_CONFIG;

	if (argc > 2) 
	{
		LOG_ERROR("too many arguments");
		return 1;
	}
	
	if (argc == 2) 
	{
		conf_path = argv[1];
	}
	LOG_INFO("reading configuration file: " << conf_path);
	ConfigParse configParser(conf_path);
	if (configParser.parseManager() == -1) {
		LOG_ERROR("configuration error, closing server...");
		return 2;
	}
	std::vector<ServerConfig> servers = configParser.getServers();
	LOG_INFO("config file loaded correctly, starting server...");
	try 
	{
		ServerCore core(servers);
		core.setup();
		core.run();
	} catch (const std::exception& e) 
	{
		LOG_ERROR("failed to init server. Error:");
		std::cerr << e.what() << std::endl;
		LOG_ERROR("closing server...");
		return 3;
	}
	return 0;
}
