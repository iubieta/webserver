// Processor.hpp
// -----------------------------------------------------------------------------

#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <string>

#include "ServerConfig.hpp"
#include "Request.hpp"
#include "Decision.hpp"
#include "LocationConfig.hpp"

class Processor {
	private:
		const ServerConfig* serv_cfg_;

		Processor(const Processor& other);
		Processor& operator=(const Processor& other);
	
		// Resolve path (decode + normalize, see path_utils.hpp)
		std::string resolvePath(const Request& req) const;

		// Check if method is allowed
		bool methodIsAllowed(const Request& req,
			const LocationConfig& cfg) const;

		// Check content-length
		// 0 => OK, continue
		// 1 => Content-length data error
		// 2 => Content-length greater than max body size
		int checkContentLength(const Request& req,
			const LocationConfig& cfg) const;

        // Configure CGI when the file extension has an interpreter in the location.
        bool setCgiHandlerIfConfigured(const std::string& disk_path,
            const LocationConfig& cfg, Decision& decision) const;

		// Define handler for the request
		int	selectHandler(const Request& req, const LocationConfig& cfg) const;

	public:
		
		Processor(const ServerConfig* serv_cfg);
		~Processor();

		Decision procRequest(const Request& req);
};

#endif // !PROCESSOR_HPP

