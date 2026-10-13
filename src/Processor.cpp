// Processor.cpp
// -----------------------------------------------------------------------------

#include "../inc/Processor.hpp"
#include <csignal>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Builders --------------------------------------------------------------------
Processor::Processor(const std::vector<ServerConfig>& cfgs):
	cfgs_(cfgs)
{}

Processor::~Processor() {};

// Private methods -------------------------------------------------------------

// Finds the specific config based on the host of the request
const ServerConfig* Processor::findConfig(const Request& req) const {
	// TODO: implement this method or return first config on the list?
	return &(*cfgs_.begin());
}


// Searchs the longest matching location with the path
const LocationConfig* Processor::findLocation(const Request& req, const ServerConfig& cfg) const {
	std::string	target = req.getPath();
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
		// Cuts one depth level from the original target
		target = target.substr(0, target.find_last_of('/'));
	}
	// If no location matches target returns NULL
	return NULL;
}

// Checks if location has a "return" parameter and in that case returns the new target
std::string Processor::redirTarget(const LocationConfig& cfg) const {
	return "";
}

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

// Public methods --------------------------------------------------------------
Decision Processor::procRequest(const Request& req) {
	Decision	res;
	
	const ServerConfig* server_cfg = findConfig(req);
	const LocationConfig* cfg = findLocation(req, *server_cfg);

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
	return res;
}
