#include "../inc/HttpParser.hpp"
#include "../inc/log_global.hpp"
#include <iostream>
#include <vector>


HttpParser::HttpParser():
	read_buffer_(""),
	state_(ESTATE_LINE),
	request_line_(NULL)
	{}

HttpParser::~HttpParser()
{
	if (request_line_ != NULL)
	{
		delete request_line_;
		ft_log::global().info("request line destructor");
	}
	ft_log::global().info("HTTP Parser destructor");	
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

e_status_code HttpParser::stateHandler(const std::string & buffer, Request &request)
{
	size_t pos;
	std::string line;
	std::vector<std::string> tokens;

	if (buffer.empty())
	{
		request.setStatus(BAD_REQUEST);
		this->state_ = BAD_REQUEST;
		return this->state_;
	}

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
					delete request_line_;
				request_line_ = new OriginForm();

				if (request_line_ ->requestLine(tokens) == -1)
				{
					this->state_ = BAD_REQUEST;
					request.setStatus(BAD_REQUEST);
					return this->state_;
				}
					
					
				request.setMethod(request_line_->getMethod());
				request.setPath(request_line_->getPath());
				request.setQuery(request_line_->getQuery());
				request.setVersion(request_line_->getVersion());
				
			}
			else
			{
				this->state_ = BAD_REQUEST;
				request.setStatus(BAD_REQUEST);
				return this->state_;
			}

			std::cout << "Method: "<< request.getMethod() << std::endl;
			std::cout << "target: "<< request.getPath() << std:: endl;
			std::cout << "query: " << request.getQuery() << std::endl;
			std::cout << "version: "<< request.getVersion() << std::endl;
		}

		
	}
	return this->state_;
}



// int main()
// {
// 	Request r;
// 	HttpParser p;

// 	p.stateHandler("GET /index", r);
// 	p.stateHandler("?=id HTTP/1.1\r\n", r);

// 	return 0;
// }