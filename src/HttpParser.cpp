#include "../inc/HttpParser.hpp"
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
		delete request_line_;
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

e_status_code HttpParser::stateHandler(const std::string & buffer)
{
	size_t pos;
	std::string line;
	std::vector<std::string> tokens;

	if (buffer.empty())
		return BAD_REQUEST;

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
		read_buffer_ = read_buffer_.erase(0, pos + 2);
		if(this->state_ == ESTATE_LINE)
		{
			RequestLine::tokenLine(line, tokens);
			if (tokens[1][0] == '/')
			{
				request_line_ = new OriginForm();
				if (request_line_ ->requestLine(tokens) == -1)
					this->state_ = BAD_REQUEST;
			}
			// std::cout << "Method: "<< request_line_->getMethod() << std::endl;
			// std::cout << "target: "<< request_line_->getPath() << std:: endl;
			// std::cout << "query: " << request_line_->getQuery() << std::endl;
			// std::cout << "version: "<< request_line_->getVersion() << std::endl;


			this->state_ = ESTATE_HEADERS;
		}
	}
	return this->state_;
}



// int main()
// {
// 	HttpParser p;

// 	p.stateHandler("GET /index");
// 	p.stateHandler("?=id HTTP/1.1\r\n");

// 	return 0;
// }