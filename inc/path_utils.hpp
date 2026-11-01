// path_utils.hpp
//
// Stateless helpers used by the Processor to resolve the request target
// -----------------------------------------------------------------------------

#ifndef PATH_UTILS_HPP
#define PATH_UTILS_HPP

#include <string>

#include "LocationConfig.hpp"
#include "ServerConfig.hpp"

namespace PathUtils {
	// Replaces %XX codification with XX hexadecimal value char
	std::string decodePath(const std::string& path);
	// Resolves "." and ".." segments. Empty string if it escapes the root
	std::string normalizePath(const std::string& path);
	// Searchs the longest matching location with the path
	const LocationConfig *findLocation(std::string target,
		const ServerConfig& cfg);
}

#endif // !PATH_UTILS_HPP
