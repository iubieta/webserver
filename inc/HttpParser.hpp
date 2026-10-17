#ifndef HTTP_PARSER_HTTP
# define HTTP_PARSER_HTTP

#include "http_status_codes.hpp"
#include "Buffer.hpp"
#include "OriginForm.hpp"
#include "Request.hpp"
#include "Headers.hpp"
#include <string>

class HttpParser
{
	private:

	e_status_code state_;
	RequestLine *request_line_; //abstract class
	Headers headers_;


	HttpParser(const HttpParser &other);
	HttpParser &operator=(const HttpParser &other);
	e_status_code reportStatusCode(e_status_code code, const std::string &log,
		Request &request);

	public:

	HttpParser();
	~HttpParser();

	void setState(e_status_code state);

	const std::string &getBuffer() const;
	e_status_code getState() const;
	e_status_code	stateHandler(Buffer &buffer, Request &request);

};
#endif