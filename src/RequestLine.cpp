#include "../inc/RequestLine.hpp"

RequestLine::RequestLine():
	request_(""),
	method_(""),
	path_(""),
	version_(""){}

RequestLine::RequestLine(const RequestLine &other):
	request_(other.request_),
	path_(other.path_),
	query_(other.query_),
	version_(other.version_){}

RequestLine &RequestLine::operator=(const RequestLine &other)
{
	if(this != &other)
	{
		request_ = other.request_;
		method_ = other.method_;
		path_ = other.path_;
		query_ = other.query_;
		version_ = other.version_;
	}

	return *this;
}

RequestLine::~RequestLine(){}

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