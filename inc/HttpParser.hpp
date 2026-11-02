#ifndef HTTP_PARSER_HTTP
# define HTTP_PARSER_HTTP

#include "http_status_codes.hpp"
#include "Buffer.hpp"
#include "OriginForm.hpp"
#include "AbsoluteForm.hpp"
#include "Request.hpp"
#include "Headers.hpp"
#include <string>

enum e_parser_state {
	LINE,
	HEADERS,
	BODY,
	CHUNK_SIZE,
	CHUNK_DATA,
	CHUNK_END,
	COMPLETE,
	ERROR,
};

class HttpParser
{
	private:

	e_parser_state state_;
	RequestLine *request_line_;
	Headers headers_;
	std::string body_;
	size_t remaining_body_bytes_;
	bool body_initialized_;
	size_t max_body_size_;
	size_t chunk_size_;


	HttpParser(const HttpParser &other);
	HttpParser &operator=(const HttpParser &other);
	e_parser_state reportStatusCode(e_status_code code, const std::string &log,
		Request &request);
	bool valueExist(const std::string &key);
	bool isValidValue(const std::string &key);
	bool isCorrectTransferEncoding();
	e_parser_state	bodyHandler(Buffer &buffer, Request &request);
	e_parser_state	bodyChunkHandler(Buffer &buffer, Request &request);
	
	public:

	HttpParser();
	~HttpParser();

	void setState(e_parser_state state);
	static bool hexadecimalToDecimal(const std::string &value, size_t &decimal);


	const std::string &getBuffer() const;
	e_parser_state getState() const;
	e_parser_state	stateHandler(Buffer &buffer, Request &request);
	e_parser_state	bodyParser(Buffer &buffer, Request &request);
	
	const std::string &getBody() const;
	size_t	getMaxbodySize() const;
	void	setMaxbodySize(size_t max_body);

};
#endif
