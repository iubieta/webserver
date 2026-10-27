// Decision.cpp
// -----------------------------------------------------------------------------

#include "../inc/Decision.hpp"
#include <string>

// Builders --------------------------------------------------------------------
Decision::Decision() :
	status_(OK),
	handler_(UNPROCESSED)
{};

Decision::Decision(const Decision &other) {
	*this = other;
}

Decision& Decision::operator=(const Decision& other) {
	if (this != &other) {
		status_				= other.status_;
		handler_			= other.handler_;
		disk_path_			= other.disk_path_;
		redir_url_			= other.redir_url_;
		allowed_methods_	= other.allowed_methods_;
	}
	return *this;
}

Decision::~Decision() {}

// Setters ---------------------------------------------------------------------
void Decision::setStatus(e_status_code status) {
	status_ = status;
}

void Decision::setHandler(e_handler handler) {
	handler_ = handler;
}

void Decision::setDiskPath(const std::string& disk_path) {
	disk_path_ = disk_path;
}

void Decision::setRedirURL(const std::string& redir_url) {
	redir_url_ = redir_url;
}

void Decision::addAllowedMethod(const std::string& method) {
	allowed_methods_.push_back(method);
}

// Getters ---------------------------------------------------------------------
e_status_code Decision::getStatus() const {
	return status_;
}

e_handler Decision::getHandler() const {
	return handler_;
}

const std::string& Decision::getDiskPath() const {
	return disk_path_;
}

const std::string& Decision::getRedirURL() const {
	return redir_url_;
}

const std::vector<std::string>& Decision::getAllowedMethods() const {
	return allowed_methods_;
}
