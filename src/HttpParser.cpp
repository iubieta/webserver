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
			{
				this->state_ = URI_TOO_LONG;
				request.setStatus(URI_TOO_LONG);

			}
			return (this->state_);
		}
		
		bytes = pos + 2;
		if (this->state_ == ESTATE_LINE && bytes > MAX_REQUEST_LINE_SIZE)
		{
			this->state_ = URI_TOO_LONG;
			request.setStatus(URI_TOO_LONG);
			return (this->state_);
		}

		line = buffer.substr(0, pos + 2);
		buffer.consume(pos + 2);

		if (this->state_ == ESTATE_LINE)
		{
			tokens.clear();
			if (RequestLine::tokenLine(line, tokens) == -1)
			{
				this->state_ = BAD_REQUEST;
				request.setStatus(BAD_REQUEST);
				return (this->state_);
			}

			if (tokens[1].empty())
			{
				this->state_ = BAD_REQUEST;
				request.setStatus(BAD_REQUEST);
				return (this->state_);
			}

			if (tokens[1][0] != '/')
			{
				this->state_ = BAD_REQUEST;
				request.setStatus(BAD_REQUEST);
				return (this->state_);
			}

			if (request_line_ != NULL)
			{
				delete request_line_;
				request_line_ = NULL;
			}
			
			request_line_ = new OriginForm();

			if (request_line_->requestLine(tokens) == -1)
			{
				this->state_ = BAD_REQUEST;
				request.setStatus(BAD_REQUEST);
				return (this->state_);
			}

			if(!request_line_->isImplementedMethod(request_line_->getMethod()))
			{
				this->state_ = NOT_IMPLEMENTED;
				request.setStatus(NOT_IMPLEMENTED);
				ft_log::global().debug("Not implemented method");
				return (this->state_);
			}
			if(!request_line_->isSupportedVersion(request_line_->getVersion()))
			{
				this->state_ = VERSION_NOT_SUPPORTED;
				request.setStatus(VERSION_NOT_SUPPORTED);
				ft_log::global().debug("Version not supported");
				return (this->state_);
			}

			request.setRequest(request_line_->getRequest());
			request.setMethod(request_line_->getMethod());
			request.setPath(request_line_->getPath());
			request.setQuery(request_line_->getQuery());
			request.setVersion(request_line_->getVersion());

			this->state_ = ESTATE_HEADERS;
		}
		else if (this->state_ == ESTATE_HEADERS)
		{
			if (line == "\r\n")
			{
				if (headers_.getHeader("host") == NULL)
				{
					ft_log::global().debug(
						"The host is missing from the request");
					request.setStatus(BAD_REQUEST);
					this->state_ = BAD_REQUEST;
					return (this->state_);
				}

				request.setHeaders(headers_.getHeaders());
				this->state_ = ESTATE_BODY;
				return (this->state_);
			}

			if (headers_.tokenHeader(line) == -1)
			{
				this->state_ = BAD_REQUEST;
				request.setStatus(BAD_REQUEST);
				return (this->state_);
			}
		}
	}

	return (this->state_);
}

void print(const std::map<std::string, std::string> &Headers)
{
	std::map<std::string, std::string>::const_iterator it;
	
	for(it = Headers.begin(); it != Headers.end(); ++it)
	{
		std::cout<< it ->first << ":" << it ->second << std::endl;
	}
}

int main()
{
	Request		request;
	HttpParser	parser;
	Buffer		buffer;

	buffer.append("GET /hastwtdvxvxhywgxwyxbzkbxxxxxxxxxxqwgkxWGLANXJAHUHCaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaindex");
	parser.stateHandler(buffer, request);

	buffer.append("QEFOHUHEUFHRUHFRUFUROSOKOO,OOVRDS33WZ,<OCKJHAXTFZZZZZZZZZZZZZAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA?=id HTTP/2.1\r\n");
	parser.stateHandler(buffer, request);

	buffer.append("content-t");
	parser.stateHandler(buffer, request);

	buffer.append("ype:  text/plain\r\n");
	parser.stateHandler(buffer, request);

	buffer.append(
		"host:localhost:8080\r\n"
		"content-length:0\r\n"
		"\r\n");
	parser.stateHandler(buffer, request);

	print(request.getHeaders());

	std::cout << "Method: "
		<< request.getMethod() << std::endl;
	std::cout << "Target: "
		<< request.getRequest() << std::endl;
	std::cout << "Path: "
		<< request.getPath() << std::endl;
	std::cout << "Query: "
		<< request.getQuery() << std::endl;
	std::cout << "Version: "
		<< request.getVersion() << std::endl;
	std::cout << "Pending bytes: "
		<< buffer.size() << std::endl;
	std::cout << "Parser state: "
		<< parser.getState() << std::endl;

	return (0);
}