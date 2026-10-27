// path_utils.cpp
// -----------------------------------------------------------------------------

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../inc/path_utils.hpp"

// PATH
// Replaces %XX codification with XX hexadecimal value char
std::string PathUtils::decodePath(const std::string& path)
{
	std::string new_path;
	for (size_t i = 0; i < path.length(); ++i) {
		if (path[i] == '%' && path.length() > (i + 2)
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

std::string PathUtils::normalizePath(const std::string& path)
{
	std::vector<std::string> stack;
	std::istringstream iss(path);
	std::string segment;
	std::string last;

	while (std::getline(iss, segment, '/')) {
		last = segment;
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
	// add last "/" when needed
	if (stack.size() > 0 ) {
		if (path[path.size() - 1] == '/' || last == "." || last == "..") {
			new_path += "/";
		}
	}
	return new_path;
}

// LOCATION
// Searchs the longest matching location with the path
const LocationConfig* PathUtils::findLocation(std::string target,
	const ServerConfig& cfg)
{
	// Search for longest matching result in the location vector
	// First iteration with the complete target
	// Later iterations alternate "/a/b" -> "/a/" -> "/a" -> "/"
	const std::vector<LocationConfig>& locs = cfg.getLocations();
	while (!target.empty())
	{
		// Iteration and comparison with config locations
		std::vector<LocationConfig>::const_iterator it;
		for (it = locs.begin(); it != locs.end(); ++it)
		{
			if (it->getTarget() == target)
			{
				return &(*it);
			}
		}
		// Cuts one depth level from the original target
		std::string temp = target;
		if (target.find_last_of('/') == target.size() - 1)
			target = target.substr(0, target.size() - 1);
		else if (target.find_last_of('/') == 0)
			target = target.substr(0, 1);
		else
			target = target.substr(0, target.find_last_of('/') + 1);
	}
	// If no location matches target returns NULL
	return NULL;
}
