// Decision.hpp
// -----------------------------------------------------------------------------

#ifndef DECISION_HPP
#define DECISION_HPP

#include "http_status_codes.hpp"
#include <map>
#include <string>
#include <vector>

// TODO: define a handler list
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
	
		std::vector<std::string>	allowed_methods_;
	
	public:
		
		// Builders
		Decision();
		Decision(const Decision &other);
		Decision& operator=(const Decision &other);
		~Decision();

		// Setters
		void setStatus(e_status_code status);
		void setHandler(e_handler handler);

		void setDiskPath(std::string& disk_path);
		void setRedirURL(std::string& redir_url);

		void addAllowedMethod(std::string method);

		// Getters
		const e_status_code getStatus() const;
		const e_handler getHandler() const;
	
		const std::string &getDiskPath() const;
		const std::string &getRedirURL() const;


};

#endif // !DECISION_HPP
