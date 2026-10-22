#include "../inc/HttpParser.hpp"
#include "../inc/log_global.hpp"
#include <iostream>
#include <vector>
#include <sstream>
#include <cstdlib> 
#include <limits> 
#include <climits>
#include <cerrno>



HttpParser::HttpParser():
	state_(ESTATE_LINE),
	request_line_(NULL),
	headers_(),
	body_(""),
	remaining_body_bytes_(0),
	body_initialized_(false),
	max_body_size_(1048576),
	chunk_size_(0)
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

const std::string &HttpParser::getBody() const
{
	return body_;
}

size_t HttpParser::getMaxbodySize() const
{
	return max_body_size_;
}

void HttpParser::setMaxbodySize(size_t max_body)
{
	this->max_body_size_ = max_body;
}

bool HttpParser::valueExist(const std::string &key)
{
	if (headers_.getHeader(key))
		return true;
	return false;
}

bool HttpParser::isValidValue(const std::string &key)
{
	const std::string	*header_value;
	size_t				i;

	header_value = headers_.getHeader(key);
	if (header_value == NULL || header_value->empty())
		return (false);
	i = 0;
	while (i < header_value->size())
	{
		if ((*header_value)[i] < '0'
			|| (*header_value)[i] > '9')
			return (false);
		++i;
	}
	return (true);
}

bool HttpParser::isCorrectTransferEncoding()
{
	const std::string *value;
	std::string temp;

	value = headers_.getHeader("transfer-encoding");
	if (value == NULL || value->empty())
		return false;
	temp = *value;
	Headers::normalize(temp);
	if (temp != "chunked")
		return false;
	return true;
}

bool HttpParser::hexadecimalToDecimal(const std::string &value, size_t &decimal)
{
	char *endptr;
	unsigned long numb;
	size_t i;

	if (value.empty())
		return (false);

	i = 0;
	while (i < value.size())
	{
		if (!((value[i] >= '0' && value[i] <= '9')
			|| (value[i] >= 'a' && value[i] <= 'f')
			|| (value[i] >= 'A' && value[i] <= 'F')))
			return (false);
		++i;
	}

	errno = 0; 
    numb = std::strtoul(value.c_str(), &endptr, 16);
    
    if (endptr == value.c_str() || *endptr != '\0')
        return false;
        
    if (errno == ERANGE)
        return false;
        
    if (numb > std::numeric_limits<size_t>::max())
        return false;
        
    decimal = static_cast<size_t>(numb);
    return true;
}


e_status_code HttpParser::reportStatusCode(e_status_code code, 
	const std::string &log, Request &request)
{
	this->state_ = code;
	request.setStatus(code);
	ft_log::global().debug(log);
	return (code);
}


e_status_code HttpParser::bodyHandler(Buffer &buffer, Request &request)
{
	size_t value;
	size_t bytes;
	std::stringstream ss;

	if (valueExist("content-length") && !body_initialized_)
	{
		if (!isValidValue("content-length"))
		{
			return reportStatusCode(BAD_REQUEST, "Invalid (content-length) value",
				request);
		}
		value = 0;
		ss.str(*headers_.getHeader("content-length"));
		ss >> value;
		if (ss.fail())
			return reportStatusCode(BAD_REQUEST, "number too large",
				request);
		remaining_body_bytes_ = value;
		if (remaining_body_bytes_ > max_body_size_)
				return reportStatusCode(PAYLOAD_TOO_LARGE, "Payload too large",
				request);
		body_initialized_ = true;
		if (remaining_body_bytes_ == 0)
		{
			this->state_ = ESTATE_COMPLETE;
			request.setBody(body_);
			return this->state_;
		}
	}

	bytes = buffer.size();
	if (bytes <= remaining_body_bytes_)
	{
		body_ += buffer.substr(0, bytes); 
		buffer.consume(bytes);
		remaining_body_bytes_ -= bytes; 
	}
	else if(bytes > remaining_body_bytes_)
	{
		body_ +=  buffer.substr(0, remaining_body_bytes_);
		buffer.consume(remaining_body_bytes_);
		remaining_body_bytes_ = 0;
	}
	if (remaining_body_bytes_ == 0)
	{
		this->state_ = ESTATE_COMPLETE;
		request.setBody(body_);
		return this->state_;
	}
	return (this->state_);
}

e_status_code HttpParser::bodyChunkHandler(Buffer &buffer, Request &request)
{
	size_t pos;
	std::string value;
	
	while(this->state_ == ESTATE_BODY || this->state_ == CHUNK_DATA 
		|| this->state_ == CHUNK_END)
	{
		if (this->state_ == ESTATE_BODY)
		{
			pos = buffer.find("\r\n");
			if (pos == std::string::npos)
				return this->state_;
			else
			{
				value = buffer.substr(0, pos);
				buffer.consume(pos + 2);
				if (!hexadecimalToDecimal(value, chunk_size_))
					return reportStatusCode(BAD_REQUEST, "Invalid chunk size",
						request);
	
			}
			if (chunk_size_ == 0)
				this->state_ = CHUNK_END;
			else if (chunk_size_ > 0)
			{
				if (remaining_body_bytes_ > max_body_size_
					|| chunk_size_ > max_body_size_ - remaining_body_bytes_)
					return (reportStatusCode(PAYLOAD_TOO_LARGE,
						"Payload too large", request));

				remaining_body_bytes_ += chunk_size_;
				this->state_ = CHUNK_DATA;

			}
		}
		if (this->state_ == CHUNK_DATA)
		{
			if (buffer.size() < chunk_size_)
				return (this->state_);

			if (buffer.size() - chunk_size_ < 2)
				return (this->state_);

			if (buffer.substr(chunk_size_, 2) != "\r\n")
				return (reportStatusCode(BAD_REQUEST,
					"Invalid chunk terminator", request));

			body_ += buffer.substr(0, chunk_size_);
			buffer.consume(chunk_size_+ 2);
			this->state_ = ESTATE_BODY;
		}
		if (this->state_ == CHUNK_END)
		{
			if (buffer.size() < 2)
				return this->state_;
			if (buffer.substr(0, 2) != "\r\n")
				return (reportStatusCode(BAD_REQUEST,
					"Invalid chunk terminator", request));

			buffer.consume(2);
			request.setBody(body_);
			this->state_ = ESTATE_COMPLETE;
		}
	}

	
	return this->state_;
	
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
				return reportStatusCode(HEADERS_TOO_LARGE, "Headers too large", request);
			
			if (this->state_ == ESTATE_HEADERS && 
				headers_.getBytesHeader() + bytes > MAX_HEADER_SIZE)
				return reportStatusCode(HEADERS_TOO_LARGE, "Headers too large", request);
			
			return (this->state_);
		}
		
		bytes = pos + 2;
		if (this->state_ == ESTATE_LINE && bytes > MAX_REQUEST_LINE_SIZE)
			return reportStatusCode(URI_TOO_LONG, "URI too large", request);
		
		if (this->state_ == ESTATE_HEADERS && bytes > MAX_HEADER_LINE_SIZE)
			return reportStatusCode(HEADERS_TOO_LARGE, "Headers too large", request);
			
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
				if (!valueExist("host"))
					return reportStatusCode(BAD_REQUEST, 
						"Bad request, The host is missing from the request", request);
				if(valueExist("content-length") && valueExist("transfer-encoding"))
					return reportStatusCode(BAD_REQUEST, 
						"Conflict between Content-Length and Transfer-Encoding", request);		
				if(valueExist("transfer-encoding"))
				{
					if (!isCorrectTransferEncoding())
						return reportStatusCode(NOT_IMPLEMENTED,
							"encoding type not implemented", request);
					this->state_ = ESTATE_BODY;
				}
				else if(valueExist("content-length"))
					this->state_ = ESTATE_BODY;
				else if(request.getMethod() == "POST")
					return (reportStatusCode(LENGTH_REQUIRED,
						"Length required", request));
				else
					this->state_ = ESTATE_COMPLETE;

				request.setHeaders(headers_.getHeaders());
				break;
			}

			if (headers_.tokenHeader(line) == -1)
				return reportStatusCode(BAD_REQUEST, "Bad request", request);

			else if(headers_.getHeaders().size() > MAX_HEADER_COUNT)
				return reportStatusCode(HEADERS_TOO_LARGE,
					"Header too large", request);

		}
	}
	if (this->state_ == ESTATE_BODY && valueExist("content-length"))
		this->state_= bodyHandler(buffer, request);
	else if(valueExist("transfer-encoding") && (this->state_ == ESTATE_BODY
		|| this->state_ == CHUNK_DATA || this->state_ == CHUNK_END))
	{
		this->state_ = bodyChunkHandler(buffer, request);
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

// 	buffer.append("POST /index HTTP/1.1\r\n");
// 	parser.stateHandler(buffer, request);

// 	//buffer.append("AAA?=id HTTP/1.1\r\n");
// 	//parser.stateHandler(buffer, request);

// 	// buffer.append("content-t");
// 	// parser.stateHandler(buffer, request);

// 	// buffer.append("ype:  text/plain\r\n");
// 	// parser.stateHandler(buffer, request);

// 	buffer.append(
// 		"host:localhost:8080\r\n"
// 		"transfer-encoding: chunked\r\n"
// 		"\r\n");
// 	parser.stateHandler(buffer, request);
	
// 	buffer.append("5\r\nhe\nlo\r\n0\r\n\r\n");
// 	parser.stateHandler(buffer, request);
// 	// buffer.append(" world/GET");
// 	// parser.stateHandler(buffer, request);
// 	// print(request.getHeaders());

// 	// std::cout << "Method: "
// 	// 	<< request.getMethod() << std::endl;
// 	// std::cout << "Target: "
// 	// 	<< request.getRequest() << std::endl;
// 	// std::cout << "Path: "
// 	// 	<< request.getPath() << std::endl;
// 	// std::cout << "Query: "
// 	// 	<< request.getQuery() << std::endl;
// 	// std::cout << "Version: "
// 	// 	<< request.getVersion() << std::endl;
// 	// std::cout << "Pending bytes: "
// 	// 	<< buffer.size() << std::endl;
// 	std::cout << "Parser state: "
// 		<< parser.getState() << std::endl;
// 	std::cout<< "body: "<<request.getBody() << std::endl;

// 	return (0);
// }