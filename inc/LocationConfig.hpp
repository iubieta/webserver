// LocationConfig.hpp
//
// Class that stocks the configuration parameters of the server
// ----------------------------------------------------------------------------

#ifndef LOCATION_CONFIG_HPP
#define LOCATION_CONFIG_HPP

#include "string"
#include "vector"
#include <sys/types.h>

class	LocationConfig {
	private:
		std::string					path;
		std::string					rooth;
		bool						autoindex;
		std::string					index;
		std::vector<std::string>	methods;
		std::string					return_;
		std::string					alias;
		std::vector<std::string>	cgi_path;
		std::vector<std::string>	cgi_ext;
		size_t						client_max_body_size;
};

#endif // !SERVER_CONFIG_HPP
