// server_core_creator.cpp
//
// File with a main that initiates a basic hardocoded server core
// ----------------------------------------------------------------------------

#include "../../inc/ServerCore.hpp"
#include "../../inc/ServerConfig.hpp"
#include "../../inc/log_global.hpp"
#include <vector>

int main() {
	std::vector<ServerConfig> cfgs;
	
	ServerConfig cfg_1;
	cfg_1.setServerName("server");
	cfg_1.setHost("localhost");
	cfg_1.setListen("8080");
	cfgs.push_back(cfg_1);
	
	ServerConfig cfg_2;
	cfg_2.setServerName("server");
	cfg_2.setHost("localhost");
	cfg_2.setListen("8081");
	cfgs.push_back(cfg_2);

	ServerCore serv(cfgs);
	log::global().debug("Setting up server...");
	serv.setup();
	log::global().debug("Running server...");
	serv.run();
}
