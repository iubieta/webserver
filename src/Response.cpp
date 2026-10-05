// Response.cpp
// -----------------------------------------------------------------------------

#include "../inc/Response.hpp"

// Builders --------------------------------------------------------------------
Response::Response() :
	version_(HTTP_V1_1),
	status_(OK),
	headers_(),
	body_()
{}

Response::Response(const Response &other) {
	*this = other;
}

Response& Response::operator=(const Response &other) {
	if (this != &other) {
		version_	= other.version_;
		status_		= other.status_;
		headers_	= other.headers_;
		body_		= other.body_;
	}
	return *this;
}

Response::~Response() {}

// Setters ---------------------------------------------------------------------
void Response::setVersion(const std::string &version) {
	version_ = version;
}

void Response::setStatus(e_status_code status) {
	status_ = status;
}

void Response::addHeader(const std::string &key, const std::string &value) {
	headers_.insert(std::pair(key, value));
}

void Response::setHeaders(const std::map<std::string, std::string> &headers) {
	headers_ = headers;
}

void Response::appendBody(const std::string &str) {
	body_.append(str);
}

void Response::setBody(const std::string &body) {
	body_ = body;
}

// Getters ---------------------------------------------------------------------
const std::string &Response::getVersion() const {
	return version_;
}

const e_status_code Response::getStatus() const {
	return status_;
}

const std::string &Response::getHeader(const std::string &key) const {
	return headers_.at(key);
}

const std::map<std::string, std::string> &Response::getHeaders() const {
	return headers_;
}

const std::string &Response::getBody() const {
	return body_;
}
