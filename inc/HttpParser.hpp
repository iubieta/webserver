#ifndef HTTP_PARSER_HTTP
# define HTTP_PARSER_HTTP

#include "http_status_codes.hpp"
#include "OriginForm.hpp"
#include "Request.hpp"
#include <string>

class HttpParser
{
	private:

	std::string read_buffer_;
	e_status_code state_;
	RequestLine *request_line_;

	HttpParser(const HttpParser &other);
	HttpParser &operator=(const HttpParser &other);


	public:

	HttpParser();
	~HttpParser();

	void setBuffer(const std::string &buffer);
	void setState(e_status_code state);

	const std::string &getBuffer() const;
	e_status_code getState() const;
	//const RequestLine *getRequestLine() const;
	
	e_status_code stateHandler(const std::string &buffer, Request &request); 

};

#endif