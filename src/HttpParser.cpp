#include "../inc/HttpParser.hpp"
#include "../inc/log_global.hpp"
#include <iostream>
#include <vector>


HttpParser::HttpParser():
	state_(ESTATE_LINE),
	request_line_(NULL),
	headers_()
	{}

HttpParser::~HttpParser()
{
	if (request_line_ != NULL)
	{
		delete request_line_;
	}
	ft_log::global().debug("HTTP Parser destructor");	
}

void HttpParser::setState(e_status_code state)
{
	this->state_ = state;
}

e_status_code HttpParser::getState() const
{
	return state_;
}

e_status_code HttpParser::reportStatusCode(e_status_code code, 
	const std::string &log, Request &request)
{
	this->state_ = code;
	request.setStatus(code);
	ft_log::global().debug(log);
	return (code);
}


e_status_code HttpParser::stateHandler(Buffer &buffer, Request &request)
{
	size_t						pos;
	size_t						bytes;
	std::string					line;
	std::vector<std::string>	tokens;
	

	if (buffer.empty())
		return (this->state_);

	while (this->state_ == ESTATE_LINE || this->state_ == ESTATE_HEADERS)
	{
		pos = buffer.find("\r\n");
		if (pos == std::string::npos)
		{
			bytes = buffer.size();
			if (this->state_ == ESTATE_LINE && bytes > MAX_REQUEST_LINE_SIZE)
				return reportStatusCode(URI_TOO_LONG, "URI too large", request);
				
			if (this->state_ == ESTATE_HEADERS && bytes > MAX_HEADER_LINE_SIZE)
				return reportStatusCode(HEADERS_TOO_LARGE, "Headers too, large", request);
			
			if (this->state_ == ESTATE_HEADERS && 
				headers_.getBytesHeader() + bytes > MAX_HEADER_SIZE)
				return reportStatusCode(HEADERS_TOO_LARGE, "Headers too large", request);
			
			return (this->state_);
		}
		
		bytes = pos + 2;
		if (this->state_ == ESTATE_LINE && bytes > MAX_REQUEST_LINE_SIZE)
			return reportStatusCode(URI_TOO_LONG, "URI too large", request);
		
		if (this->state_ == ESTATE_HEADERS && bytes > MAX_HEADER_LINE_SIZE)
			return reportStatusCode(HEADERS_TOO_LARGE, "Headres too large", request);
			
		line = buffer.substr(0, bytes);
		buffer.consume(bytes);

		if (this->state_ == ESTATE_LINE)
		{
			tokens.clear();
			if (RequestLine::tokenLine(line, tokens) == -1)
				return reportStatusCode(BAD_REQUEST, "Bad request", request);
			
			if (tokens[1].empty())
				return reportStatusCode(BAD_REQUEST, "Bad request", request);

			if (tokens[1][0] != '/')
				return reportStatusCode(BAD_REQUEST, "Bad request", request);

			if (request_line_ != NULL)
			{
				delete request_line_;
				request_line_ = NULL;
			}
			
			request_line_ = new OriginForm();

			if (request_line_->requestLine(tokens) == -1)
				return reportStatusCode(BAD_REQUEST, "Bad request", request);

			if(!request_line_->isImplementedMethod(request_line_->getMethod()))
				return reportStatusCode(NOT_IMPLEMENTED,"Not implemented method",
					request);
			
			if(!request_line_->isSupportedVersion(request_line_->getVersion()))
				return reportStatusCode(VERSION_NOT_SUPPORTED,
					"Version not supported", request);
			
			request.setRequest(request_line_->getRequest());
			request.setMethod(request_line_->getMethod());
			request.setPath(request_line_->getPath());
			request.setQuery(request_line_->getQuery());
			request.setVersion(request_line_->getVersion());

			this->state_ = ESTATE_HEADERS;
		}
		else if (this->state_ == ESTATE_HEADERS)
		{
	
			if (headers_.addBytesHeader(line.size()) > MAX_HEADER_SIZE)
				return reportStatusCode(HEADERS_TOO_LARGE,
					"Header too large", request);

			if (line == "\r\n")
			{
				if (headers_.getHeader("host") == NULL)
					return reportStatusCode(BAD_REQUEST, 
						"Bad request, The host is missing from the request", request);

				request.setHeaders(headers_.getHeaders());
				this->state_ = ESTATE_BODY;
				return (this->state_);
			}

			if (headers_.tokenHeader(line) == -1)
				return reportStatusCode(BAD_REQUEST, "Bad request", request);

			else if(headers_.getHeaders().size() > MAX_HEADER_COUNT)
				return reportStatusCode(HEADERS_TOO_LARGE,
					"Header too large", request);

		}
	}

	return (this->state_);
}

// void print(const std::map<std::string, std::string> &Headers)
// {
// 	std::map<std::string, std::string>::const_iterator it;
	
// 	for(it = Headers.begin(); it != Headers.end(); ++it)
// 	{
// 		std::cout<< it ->first << ":" << it ->second << std::endl;
// 	}
// }

// int main()
// {
// 	Request		request;
// 	HttpParser	parser;
// 	Buffer		buffer;

// 	buffer.append("GET /index");
// 	parser.stateHandler(buffer, request);

// 	buffer.append("AAA?=id HTTP/1.1\r\n");
// 	parser.stateHandler(buffer, request);

// 	buffer.append("content-t");
// 	parser.stateHandler(buffer, request);

// 	buffer.append("ype:  text/plain\r\n");
// 	parser.stateHandler(buffer, request);

// 	buffer.append(
// 		"host:localhost:8080\r\n"
// 		"content-length:0\r\n"
// 		"\r\n");
// 	parser.stateHandler(buffer, request);

// 	print(request.getHeaders());

// 	std::cout << "Method: "
// 		<< request.getMethod() << std::endl;
// 	std::cout << "Target: "
// 		<< request.getRequest() << std::endl;
// 	std::cout << "Path: "
// 		<< request.getPath() << std::endl;
// 	std::cout << "Query: "
// 		<< request.getQuery() << std::endl;
// 	std::cout << "Version: "
// 		<< request.getVersion() << std::endl;
// 	std::cout << "Pending bytes: "
// 		<< buffer.size() << std::endl;
// 	std::cout << "Parser state: "
// 		<< parser.getState() << std::endl;

// 	return (0);
// }