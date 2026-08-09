// ServerConfig.hpp
// 
// Class that stocks the configuration parameters of the server
// ----------------------------------------------------------------------------

#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <map>
#include <string>
#include <cstdint>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "LocationConfig.hpp"

class	ServerConfig {
	private:
		uint16_t							listen_port;
		uint32_t 							host;
		std::string							server_name;
		struct sockaddr_in					server_addr;
		std::string							root;
		size_t								client_max_body_size;
		bool								autoindex;
		std::string							index;
		std::map<short, std::string>		error_pages;
		std::vector<LocationConfig>			locations;
};

#endif // !SERVER_CONFIG_HPP
