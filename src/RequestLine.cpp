#include "../inc/RequestLine.hpp"
#include "../inc/log_global.hpp"
#include "../inc/Logger.hpp"
#include <iostream>
#include <vector>

RequestLine::RequestLine():
	method_(""),
	path_(""),
	query_(""),
	version_(""),
	request_(""){}

RequestLine::RequestLine(const RequestLine &other):
	method_(other.method_),
	path_(other.path_),
	query_(other.query_),
	version_(other.version_),
	request_(""){}

RequestLine &RequestLine::operator=(const RequestLine &other)
{
	if(this != &other)
	{
		
		method_ = other.method_;
		path_ = other.path_;
		query_ = other.query_;
		version_ = other.version_;
		request_ = other.request_;
	}

	return *this;
}

RequestLine::~RequestLine(){}


bool RequestLine::isCharacterInvalid(char c)
{
	if (c == '\t' || c == '\n' || c == '\r' || 
		(c >= 33 && c <= 47) || ( c >= 58 && c <= 64)
		|| (c >= 123 && c <= 126))
		return 1;
	return 0;
}

int RequestLine::tokenLine(const std::string &line)
{
	size_t i;
	size_t start;
	std::vector<std::string> token;

	if(line.empty())
	{
		ft_log::global().info(
			"RequestLine: empty string", __FILE__, __LINE__);
		return -1;
	}
	
	if (line[0] == ' ' || isCharacterInvalid(line[0]))
	{
		ft_log::global().info(
			"RequestLine: Invalid first character", __FILE__, __LINE__);
		return -1;
	}

	i = 0;
	while(i < line.size())
	{
		start = i;
		while(i < line.size() && line[i] != ' ')
			i++;
		token.push_back(line.substr(start, i - start));

		while (i < line.size() && line[i] == ' ')
			i++;
		
	}

	if (token.size() != 3)
	{
		ft_log::global().info(
			"Requestline: incorrect token number", __FILE__, __LINE__);
		return (-1);
	}

	i = token[2].size();
	if(token[2][i - 1] != '\n' || token[2][i - 2] != '\r')
	{
		ft_log::global().info(
			"Requestline: Incorrect line ending", __FILE__, __LINE__);
		return -1;
	}

	method_= token[0];
	request_ = token[1];
	version_ = token[2].substr(0, token[2].size() - 2);

	return 0;
	
}

bool RequestLine::isValidMethod(const std::string &method) const
{
	size_t i;

	i = 0;
	while (i < method.size())
	{
		if(isCharacterInvalid(method[i]))
			return 0;
		i++;
	}
	return 1;
}

bool RequestLine::isVaildVersion(const std::string &version) const
{
	if (version.size() != 8)
		return 0;
	if (version[0] != 'H' || version[1] != 'T' || version[2] != 'T' ||
		version[3] != 'P' || version[4] != '/' || version[6] != '.')
		return 0;
	if (version[5] < 49 || version[5] > 57 || version[7] < 49 || version[7] > 59)
		return 0;
	return 1;
}

void RequestLine::setMethod(const std::string &method)
{
	this->method_ = method;
}

void RequestLine::setPath(const std::string &path)
{
	this->path_ = path;
}

void RequestLine::setVersion(const std::string &version)
{
	this->version_ = version;
}

const std::string &RequestLine::getRequest() const
{
	return this->request_;
}

const std::string &RequestLine::getMethod() const
{
	return this->method_;
}

const std::string &RequestLine::getPath() const
{
	return this->path_;
}

const std::string &RequestLine::getVersion() const
{
	return this->version_;
}