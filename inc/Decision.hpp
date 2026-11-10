// Decision.hpp
// -----------------------------------------------------------------------------

#ifndef DECISION_HPP
#define DECISION_HPP

#include "ServerConfig.hpp"
#include "http_status_codes.hpp"
#include <map>
#include <string>
#include <vector>

// Forward declarations to avoid circular includes
class LocationConfig;

enum e_handler {
	UNPROCESSED,
	READ_FILE,		// Requires disk_path
	WRITE_FILE,		// Requires disk_path
	REMOVE_FILE,	// Requires disk_path
	LIST_DIR,		// Requires disk_path
	REDIRECTION,	// Requires redir_url
	ERROR_PAGE		// Requires status always and the rest sometimes
};

class Decision {
	private:
		e_status_code	status_;
		e_handler		handler_;
		
		std::string		disk_path_;
		std::string		redir_url_;
		std::string		error_page_;
	
		std::vector<std::string>	allowed_methods_;

		const ServerConfig*		server_cfg_;
		const LocationConfig*	location_;
	
	public:
		
		// Builders
		Decision();
		Decision(const Decision &other);
		Decision& operator=(const Decision& other);
		~Decision();

		// Setters
		void setStatus(e_status_code status);
		void setHandler(e_handler handler);

		void setDiskPath(const std::string& disk_path);
		void setRedirURL(const std::string& redir_url);
		void setErrorPage(const std::string& error_page);
		void setError(e_status_code error);

		void addAllowedMethod(const std::string& method);

		void setServerConfig(const ServerConfig* server_config);
		void setLocationConfig(const LocationConfig* location_config);

		// Getters
		e_status_code getStatus() const;
		e_handler getHandler() const;
	
		const std::string &getDiskPath() const;
		const std::string &getRedirURL() const;
		const std::string &getErrorPage() const;
		const std::vector<std::string> &getAllowedMethods() const;

		const ServerConfig* getServerConfig() const;
		const LocationConfig* getLocationConfig() const;


};

#endif // !DECISION_HPP
