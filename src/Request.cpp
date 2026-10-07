// Request.cpp
// -----------------------------------------------------------------------------

#include "../inc/Request.hpp"
#include "../inc/log_global.hpp"
#include <string>

// Builders --------------------------------------------------------------------
Request::Request() : 
	request_(), status_(ESTATE_LINE), 
	method_(), path_(), query_(), version_(),
	headers_(),
	body_()
{}

Request::Request(const Request &other) {
	*this = other;
}

Request& Request::operator=(const Request &other) {
	if (this != &other) {
		request_	= other.request_;
		status_		= other.status_;
		
		method_		= other.method_;
		path_		= other.method_;
		query_		= other.query_;
		version_	= other.version_;

		headers_	= other.headers_;
		body_		= other.body_;
	}
	return *this;
}

Request::~Request() 
{
	ft_log::global().info("request destructor");
}

// Setters ---------------------------------------------------------------------
void Request::setRequest(const std::string &request) {
	request_ = request;
}

void Request::setStatus(e_status_code status) {
	status_ = status;
}

void Request::setMethod(const std::string &method) {
	method_ = method;
}

void Request::setPath(const std::string &path) {
	path_ = path;
}

void Request::setQuery(const std::string &query) {
	query_ = query;
}

void Request::setVersion(const std::string &version) {
	version_ = version;
}

void Request::addHeader(const std::string &key, const std::string &value) {
	headers_.insert(std::pair(key, value));
}

void Request::setHeaders(const std::map<std::string, std::string> &headers) {
	headers_ = headers;
}

void Request::appendBody(const std::string &str) {
	body_.append(str);
}

void Request::setBody(const std::string &body) {
	body_ = body;
}

// Getters ---------------------------------------------------------------------
const std::string &Request::getRequest() const {
	return request_;
}

const e_status_code Request::getStatus() const {
	return status_;
}

const std::string &Request::getMethod() const {
	return method_;
}

const std::string &Request::getPath() const {
	return path_;
}

const std::string &Request::getQuery() const {
	return query_;
}

const std::string &Request::getVersion() const {
	return version_;
}

const std::string &Request::getHeader(const std::string &key) const {
	return headers_.at(key);
}

const std::map<std::string, std::string> &Request::getHeaders() const {
	return headers_;
}

const std::string &Request::getBody() const {
	return body_;
}
