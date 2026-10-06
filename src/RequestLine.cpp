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
	request_(other.request_){}

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
	if ((c >= 'A' && c <= 'Z')
		|| (c >= 'a' && c <= 'z')
		|| (c >= '0' && c <= '9'))
		return (false);
	if (c == '!' || c == '#' || c == '$' || c == '%'
		|| c == '&' || c == '\'' || c == '*'
		|| c == '+' || c == '-' || c == '.'
		|| c == '^' || c == '_' || c == '`'
		|| c == '|' || c == '~')
		return (false);
	return (true);
}

int RequestLine::tokenLine(const std::string &line, std::vector<std::string> &token)
{
	size_t i;
	size_t start;
	std::string temp;

	if(line.empty())
	{
		ft_log::global().info(
			"RequestLine: empty string", __FILE__, __LINE__);
		return -1;
	}
	
	if (isCharacterInvalid(line[0]))
	{
		ft_log::global().info(
			"RequestLine: Invalid first character", __FILE__, __LINE__);
		return -1;
	}

	i = line.size();
	if (i < 2 || line[i - 2] != '\r'
					|| line[i - 1] != '\n')
	{
		ft_log::global().info(
			"Requestline: Incorrect line ending", __FILE__, __LINE__);
		return -1;
	}

	temp = line.substr(0,line.size() - 2);
	i = 0;
	while(i < temp.size())
	{
		start = i;
		while(i < temp.size() && temp[i] != ' ')
			i++;
		token.push_back(temp.substr(start, i - start));

		while (i < temp.size() && temp[i] == ' ')
			i++;
		
	}
	return 0;
	
}

bool RequestLine::isValidMethod(const std::string &method) const
{
	size_t	i;

	if (method.empty())
		return (false);
	i = 0;
	while (i < method.size())
	{
		if (isCharacterInvalid(method[i]))
			return (false);
		i++;
	}
	return (true);
}

bool RequestLine::isVaildVersion(const std::string &version) const
{
	if (version.size() != 8)
		return false;
	if (version[0] != 'H' || version[1] != 'T' || version[2] != 'T' ||
		version[3] != 'P' || version[4] != '/' || version[6] != '.')
		return false;
	if (version[5] < '0' || version[5] > '9' || version[7] < '0' || version[7] > '9')
		return false;
	return true;
}

void RequestLine::setMethod(const std::string &method)
{
	this->method_ = method;
}

void RequestLine::setRequest(const std::string &request)
{
	this->request_ = request;
}

void RequestLine::setPath(const std::string &path)
{
	this->path_ = path;
}

void RequestLine::setQuery(const std::string &query)
{
	this->query_ = query;
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

const std::string &RequestLine::getQuery() const
{
	return this -> query_;
}