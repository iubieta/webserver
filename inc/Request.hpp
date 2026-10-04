// Request.hpp
// ----------------------------------------------------------------------------

#include <map>
#include <shared_mutex>
#include <string>
#ifndef REQUEST_HPP

#include "http_status_codes.h"

class Request 
{
	private:
		std::string		request_;
		e_status_code		status_;
	
		std::string		method_;
		std::string		path_;
		std::string		query_;
		std::string		version_;
	
		std::map<std::string, std::string>	headers_;
	
		std::string		body_;
	
	public:
		// Builders
		Request();
		Request(const Request &other);
		Request& operator=(const Request &other);
		~Request();
	
		// Setters
		void setRequest(const std::string &request);
		void setStatus(e_status_code status);
	
		void setMethod(const std::string &method);
		void setPath(const std::string &path);
		void setQuery(const std::string &query);
		void setVersion(const std::string &version);
	
		void addHeader(const std::string key, const std::string &value);
		void setHeaders(const std::map<std::string, std::string> &headers);
	
		void appendBody(const std::string &str);
		void setBody(const std::string &body);
		
	
		// Getters
		const std::string &getRequest() const;
		const e_status_code getStatus() const;
	
		const std::string &getMethod() const;
		const std::string &getPath() const;
		const std::string &getQuery() const;
		const std::string &getVersion() const;
	
		const std::string &getHeader(const std::string &key) const;
		const std::map<std::string, std::string> &getHeaders() const;
	
		const std::string &getBody() const;
};
 
#endif // !REQUEST_HPP
