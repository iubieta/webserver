// listening_socket_creator.cpp
//
// File with a main that initiates a listening port based on the 
// ListeningSocket class
// ----------------------------------------------------------------------------

#include "../../inc/ListeningSocket.hpp"
#include <iostream>
#include <unistd.h>

enum Mode { HOLD, ACCEPT, SCOPE };

int strToMode(const std::string &str) {
	if (str == "hold")
		return HOLD;
	if (str == "accept")
		return ACCEPT;
	if (str == "scope")
		return SCOPE;
	return -1;
}

int main(int argc, char **argv) {
	if (argc != 5) {		
		std::cerr << "ERROR: check your arguments" << std::endl;
		return 1;
	}
	std::string host(argv[1]);
	int			port = atoi(argv[2]);
	std::string flag(argv[3]);
	std::string mode_str(argv[4]);
	if (flag != "--mode" || 
		!(mode_str == "hold" || mode_str == "accept" || mode_str == "scope")) {
		std::cerr << "ERROR: check your mode arguments" << std::endl;
		return 1;
	}
	int mode = strToMode(mode_str);
	switch (mode) {
		case HOLD:
			{
				std::cout << "--- Hold Mode ---"<< std::endl;
				ListeningSocket ls(host, port);
				ls.setup();
				if (ls.isReady())
					std::cout << "SETUP=OK" << std::endl;
				else {
					std::cout << "SETUP=KO" << std::endl;
					return 1;
				}
				while (true) ;
				break;
			}
		case ACCEPT:
			{
				std::cout << "--- Accept Mode ---"<< std::endl;
				ListeningSocket ls(host, port);
				ls.setup();
				if (ls.isReady())
					std::cout << "SETUP=OK" << std::endl;
				while (true) {
					usleep(100);
					int fd = ls.acceptClient();
					if (fd != -1) {
						std::cout << "ACCEPT=" << fd << std::endl;
					}
				}
				break;
			}
		case SCOPE: 
			{
				std::cout << "--- Scope Mode ---"<< std::endl;
				ListeningSocket ls(host, port);
				ls.setup();
				if (ls.isReady())
					std::cout << "SETUP=OK" << std::endl;
			}
			std::cout << "DESTROYED" << std::endl;
			while (true) ;
	}
	return 0;
}
