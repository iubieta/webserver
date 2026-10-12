// Processor.hpp
// -----------------------------------------------------------------------------

#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include "../inc/ServerConfig.hpp"
#include "../inc/Request.hpp"
#include "../inc/Decision.hpp"
#include "LocationConfig.hpp"
#include "http_status_codes.hpp"
#include <string>
#include <vector>

class Processor {
	private:
		// TODO: aclarar si es solo una config o un array de configs
		// como se contempla configs distintas con un mismo puerto?
		const std::vector<ServerConfig>& cfgs_;

		Processor(const Processor& other);
		Processor& operator=(const Processor& other);
	
		// Find config for the host
		const ServerConfig& findConfig(const Request& req);
		// Find mathcing location for the server
		const LocationConfig& findLocation(const Request& req, const ServerConfig& cfg) const;
		// Check if location has a return
		std::string redirTarget(const LocationConfig& cfg) const;
		// Check if method is allowed
		bool methodIsAllowed(const Request& req, const LocationConfig& cfg) const;
		// Check content-length
		bool checkContentLength(const Request& req, const LocationConfig& cfg) const;
		// Resolve path
		std::string resolvePath(const Request& req, const LocationConfig& cfg) const;
		// Resolve error page path
		std::string findErrorPage(e_status_code code, const LocationConfig& cfg) const;
		// Define handler for the request
		// TODO: define a handler list
		int	selectHandler(const Request& req, const LocationConfig& cfg) const;


	public:
		Processor(const std::vector<ServerConfig >& cfgs);
		~Processor();

		Decision procRequest(const Request& req);
};

#endif // !PROCESSOR_HPP

