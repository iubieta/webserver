#include "../inc/HttpParser.hpp"
#include "../inc/log_global.hpp"
#include <iostream>
#include <vector>


HttpParser::HttpParser():
	read_buffer_(""),
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

void HttpParser::setBuffer(const std::string &buffer)
{
	this->read_buffer_ = buffer;
}

e_status_code HttpParser::getState() const
{
	return state_;
}

const std::string &HttpParser::getBuffer() const
{
	return read_buffer_;
}

e_status_code HttpParser::stateHandler(const std::string &buffer, Request &request)
{
	size_t pos;
	std::string line;
	std::vector<std::string> tokens;

	if (buffer.empty())
		return (this->state_);

	if (read_buffer_.empty())
		read_buffer_ = buffer;
	else
		read_buffer_.append(buffer);
	pos = read_buffer_.find("\r\n");
	if (pos == std::string::npos)
		return this->state_;
	else
	{
		line = read_buffer_.substr(0, pos + 2);
		read_buffer_.erase(0, pos + 2);
		while(!line.empty())
		{
			if(this->state_ == ESTATE_LINE)
			{
				if (RequestLine::tokenLine(line, tokens) == -1)
				{
					this->state_ = BAD_REQUEST;
					request.setStatus(BAD_REQUEST);
					return this->state_;
				}

				if (tokens[1].empty())
				{
					this->state_ = BAD_REQUEST;
					request.setStatus(BAD_REQUEST);
					return this->state_;
				}

				if (tokens[1][0] == '/')
				{
					if (request_line_ != NULL)
					{
						delete request_line_;
						request_line_ = NULL;
					}
					request_line_ = new OriginForm();
					if (request_line_ ->requestLine(tokens) == -1)
					{
						this->state_ = BAD_REQUEST;
						request.setStatus(BAD_REQUEST);
						return this->state_;
					}
					
					request.setRequest(request_line_->getRequest()); //target	
					request.setMethod(request_line_->getMethod());
					request.setPath(request_line_->getPath());
					request.setQuery(request_line_->getQuery());
					request.setVersion(request_line_->getVersion());

					this->state_ = ESTATE_HEADERS;
				
				}
				else
				{
					this->state_ = BAD_REQUEST;
					request.setStatus(BAD_REQUEST);
					return (this->state_);
				}
			

				std::cout << "Method:"<< request.getMethod() << std::endl;
				std::cout << "target:"<< request.getPath() << std:: endl;
				std::cout << "query:" << request.getQuery() << std::endl;
				std::cout << "version:"<< request.getVersion() << std::endl;
			}
			else if(this->state_ == ESTATE_HEADERS)
			{
				if (line == "\r\n")
				{
					if (headers_.getHeader("host") == NULL)
					{
						ft_log::global().debug(
							"The host is missing from the request"
						);
						request.setStatus(BAD_REQUEST);
						this->state_ = BAD_REQUEST;
						return (this->state_);
					}
					request.setHeaders(headers_.getHeaders());
					this -> state_ = ESTATE_BODY;
					return (this->state_);
				}
				else if (headers_.tokenHeader(line) == -1)
				{
					this -> state_ = BAD_REQUEST;
					request.setStatus(BAD_REQUEST);
					return this -> state_;
				}
				
			}
			else
			{
				this->state_ = BAD_REQUEST;
				request.setStatus(BAD_REQUEST);
				return this->state_;
			}

			pos = read_buffer_.find("\r\n");
			if (pos != std::string::npos)
			{
				line = read_buffer_.substr(0, pos + 2);
				read_buffer_.erase(0, pos + 2);
			}
			else
				line = "";
		}
		

	}
	return this->state_;
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
	Request r;
	HttpParser p;

	p.stateHandler("GET /index", r);
	p.stateHandler("?=id HTTP/1.1\r\n", r);
	p.stateHandler("content-t", r);
	p.stateHandler("ype:  text/plain\r\n", r);
	p.stateHandler(
	"host:localhost:8080\r\n"
	"content-length:0\r\n\r\n",
	r);

	

	print(r.getHeaders());

	return 0;
}