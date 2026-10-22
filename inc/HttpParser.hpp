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
	std::string body_;
	size_t remaining_body_bytes_;
	bool body_initialized_;
	size_t max_body_size_;
	size_t chunk_size_;


	HttpParser(const HttpParser &other);
	HttpParser &operator=(const HttpParser &other);
	e_status_code reportStatusCode(e_status_code code, const std::string &log,
		Request &request);
	bool valueExist(const std::string &key);
	bool isValidValue(const std::string &key);
	bool isCorrectTransferEncoding();
	e_status_code	bodyHandler(Buffer &buffer, Request &request);
	e_status_code	bodyChunkHandler(Buffer &buffer, Request &request);
	
	public:

	HttpParser();
	~HttpParser();

	void setState(e_status_code state);
	static bool hexadecimalToDecimal(const std::string &value, size_t &decimal);


	const std::string &getBuffer() const;
	e_status_code getState() const;
	e_status_code	stateHandler(Buffer &buffer, Request &request);
	
	const std::string &getBody() const;
	size_t	getMaxbodySize() const;
	void	setMaxbodySize(size_t max_body);

};
#endif