// Processor.cpp
// -----------------------------------------------------------------------------

#include "../inc/Processor.hpp"
#include <cctype>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <strings.h>
#include <vector>

// Builders --------------------------------------------------------------------
// The builder has the server configs corresponding to the connection as input
Processor::Processor(const std::vector<ServerConfig>& cfgs):
	cfgs_(cfgs)
{}

Processor::~Processor() {}

// Private methods -------------------------------------------------------------

// CONFIG AND LOCATION
// Finds the specific config based on the host of the request (virtual hosts)
const ServerConfig* Processor::findConfig(const Request& req) const {
	// OPTIONAL: implement this method
	// Default = first config
	return &(*cfgs_.begin());
}

// PATH
// Resolves the whole path to useful one
std::string Processor::resolvePath(const Request& req) const
{
	std::string new_path = decodePath(req.getPath());
	new_path = normalizePath(new_path);
	return new_path;
}

// Replaces %XX codification with XX hexadecimal value char
std::string Processor::decodePath(const std::string& path) const
{
	std::string new_path;
	for (size_t i = 0; i < path.length(); ++i) {
		if (path[i] == '%' && path.length() >= (i + 2)
			&& std::isxdigit(static_cast<unsigned char>(path[i + 1]))
			&& std::isxdigit(static_cast<unsigned char>(path[i + 2])) )
		{
			std::string hex = path.substr(i + 1, 2);
			new_path += static_cast<char>(std::strtol(hex.c_str(), NULL, 16));
			i += 2;
		}
		else {
			new_path += path[i];
		}
	}
	return  new_path;
}

std::string Processor::normalizePath(const std::string& path) const
{
	std::vector<std::string> stack;
	std::istringstream iss(path);
	std::string segment;

	while (std::getline(iss, segment, '/')) {
		if (segment == "." || segment.empty() )
			continue;
		if (segment == "..") {
			if (stack.empty())
				return "";		// Empty string as ERROR
			stack.pop_back();
			continue;
		}
		stack.push_back(segment);
	}
	
	std::string new_path = "/";
	for (size_t i = 0; i < stack.size(); ++i) {
		new_path += stack[i];
		if (i + 1 < stack.size())
			new_path += "/";
	}
	// TODO: add last "/" when needed
	// if (path.size() > 1 && path[path.size() - 1] == '/')
	// 	new_path += "/";
	return new_path;
}

// LOCATION
// Searchs the longest matching location with the path
const LocationConfig* Processor::findLocation(std::string target, const ServerConfig& cfg) const {
	// Search for longest matching result in the location vector
	// First iteration with the complete target
	// Later iterations one "/" less each time
	while (!target.empty()) 
	{
		// Iteration and comparison with config locations
		std::vector<LocationConfig>::const_iterator it;
		std::vector<LocationConfig>::const_iterator ite = cfg.getLocations().end();
		for (it = cfg.getLocations().begin(); it != ite; ++it) 
		{
			if (it->getTarget() == target)
			{
				return &(*it);
			}
		}
		if (target.size() == 1)
			break;
		// Cuts one depth level from the original target
		target = target.substr(0, target.find_last_of('/') + 1);
	}
	// If no location matches target returns NULL
	return NULL;
}

// REDIR
// Checks if location has a "return" parameter and in that case returns the new target
std::string Processor::redirTarget(const LocationConfig& cfg) const {
	return "";
}

// ALLOWED METHODS
// Check if the specified method is allowed in the configuration
bool Processor::methodIsAllowed(const Request& req, const LocationConfig& cfg) const {
	const std::string& method = req.getMethod();
	const std::vector<std::string>& methods = cfg.getMethods();
	std::vector<std::string>::const_iterator it;
	std::vector<std::string>::const_iterator ite = cfg.getMethods().end();
	for (it = cfg.getMethods().begin(); it != ite; ++it) {
		if (*it == method) {
			return true;
		}
	}
	return false;
}

// CONTENT-LENGTH
// Check if content-length is greater than max-body-size
bool Processor::checkContentLength(const Request& req, const LocationConfig& cfg) const {
	return true;
}

// Public methods --------------------------------------------------------------
Decision Processor::procRequest(const Request& req) {
	Decision	res;
	
	const ServerConfig* server_cfg = findConfig(req);
	
	std::string target = resolvePath(req);
	
	const LocationConfig* cfg = findLocation(target, *server_cfg);
	if (cfg == NULL) 
	{
		// TODO: what should be done if no location is found??
		res.setStatus(FORBIDDEN);
		res.setHandler(ERROR_PAGE);
		return res;
	}
	
	if (!methodIsAllowed(req, *cfg))
	{
		res.setStatus(FORBIDDEN);
		res.setHandler(ERROR_PAGE);
	}

	// DISK PATH
	std::string disk_path = cfg->getRoot() + target;
	res.setDiskPath(disk_path);

	if (!checkContentLength(req, *cfg))
	{
	}

	return res;
}

