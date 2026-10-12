// Processor.cpp
// -----------------------------------------------------------------------------

#include "../inc/Processor.hpp"
#include <iostream>
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


// Search the longest matching location with the path
const LocationConfig* Processor::findLocation(const Request& req, const ServerConfig& cfg) const {
	std::string	target = req.getPath();
	const std::vector<LocationConfig> location_vec = cfg.getLocations();
	// Search for longest matching result in the location vector
	// First iteration with the complete target
	// Later iterations one "/" less each time
	while (!target.empty()) 
	{
		// Iteration and comparison with config locations
		std::vector<LocationConfig>::const_iterator it;
		std::vector<LocationConfig>::const_iterator ite = location_vec.end();
		for (it = location_vec.begin(); it < ite; ++it) 
		{
			if (it->getTarget() == target)
			{
				std::cout << "Found location: "
					<< it->getTarget() << std::endl;
				return &(*it);
			}
		}
		// Cuts one depth level from the original target
		target = target.substr(0, target.find_last_of('/'));
	}
	// If no location matches target returns NULL
	return NULL;
}


