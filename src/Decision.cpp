// Decision.cpp
// -----------------------------------------------------------------------------

#include "../inc/Decision.hpp"
#include "../inc/path_utils.hpp"
#include <string>

// Builders --------------------------------------------------------------------
Decision::Decision() :
	status_(OK),
	handler_(UNPROCESSED),
	server_cfg_(NULL),
	location_(NULL)
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
		error_page_			= other.error_page_;
		allowed_methods_	= other.allowed_methods_;
		server_cfg_			= other.server_cfg_;
		location_			= other.location_;
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

void Decision::setErrorPage(const std::string& error_page) {
	error_page_ = error_page;
}

void Decision::setError(e_status_code error) {
	status_ = error;
	handler_ = ERROR_PAGE;
	error_page_ = PathUtils::resolveErrorPage(error, location_, server_cfg_);
}

void Decision::addAllowedMethod(const std::string& method) {
	allowed_methods_.push_back(method);
}

void Decision::setServerConfig(const ServerConfig* server_config) {
	server_cfg_ = server_config;
}

void Decision::setLocationConfig(const LocationConfig* location_config) {
	location_ = location_config;
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

const std::string& Decision::getErrorPage() const {
	return error_page_;
}

const std::vector<std::string>& Decision::getAllowedMethods() const {
	return allowed_methods_;
}

const ServerConfig* Decision::getServerConfig() const {
	return server_cfg_;
}

const LocationConfig* Decision::getLocationConfig() const {
	return location_;
}
