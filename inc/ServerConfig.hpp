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

class	ServerConfig 
{
	private:
		unsigned int						listen_;
		unsigned int						host_;
		std::string							server_name_;
		std::string							root_;
		size_t								client_max_body_size_;
		bool								autoindex_;
		std::vector<std::string>			index_;
		std::vector<LocationConfig>			locations_;
		//std::map<int, std::string>			error_pages_;

	public:
		ServerConfig();
		ServerConfig(const ServerConfig &other);
		ServerConfig &operator=(const ServerConfig &other);
		~ServerConfig();

		const unsigned int &getListen() const;
		const unsigned int &getHost() const;
		const std::string &getServerName() const;
		const std::string &getRoot() const;
		const size_t &getClientMaxBody() const;
		const bool &getAutoindex() const;

		void setListen(unsigned int listen);
		void setHost(unsigned int host);
		void setServerName(const std::string &server_name);
		void setRoot(const std::string &root);
		void setClientMax(const size_t client_max_body);
		void setAutoindex(bool autoindex);

};

#endif // !SERVER_CONFIG_HPP
