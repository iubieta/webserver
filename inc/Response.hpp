// Response.hpp
// ----------------------------------------------------------------------------

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "http_status_codes.hpp"
#include <map>
#include <string>

#define HTTP_V1_1 "HTTP/1.1"

class Response 
{
	private:
		std::string		version_;
		e_status_code	status_;
	
		std::map<std::string, std::string>	headers_;
		
		std::string		body_;

	
	public:
		
		// Builders
		Response();
		Response(const Response &other);
		Response& operator=(const Response &other);
		~Response();

		// Setters
		void setVersion(const std::string &version);
		void setStatus(e_status_code status);
		void setStatusMsg(const std::string &status_msg);

		void addHeader(const std::string &key, const std::string &value);
		void setHeaders(const std::map<std::string, std::string> &headers);

		void appendBody(const std::string &str);
		void setBody(const std::string &body);

		// Getters
		const std::string &getVersion() const;
		const e_status_code getStatus() const;
	
		const std::string &getHeader(const std::string &key) const;
		const std::map<std::string, std::string> &getHeaders() const;
	
		const std::string &getBody() const;
};

#endif // !RESPONSE_HPPi
