// Processor.cpp
// -----------------------------------------------------------------------------

#include <string>
#include <vector>

#include "../inc/Processor.hpp"

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
	(void)req;
	// Default = first config
	if (cfgs_.empty())
		return NULL;
	return &(*cfgs_.begin());
}

// PATH
// Resolves the whole path to useful one
std::string Processor::resolvePath(const Request& req) const
{
	std::string new_path = PathUtils::decodePath(req.getPath());
	new_path = PathUtils::normalizePath(new_path);
	return new_path;
}

// REDIR
// Checks if location has a "return" parameter and in that case returns the new target
std::string Processor::redirTarget(const LocationConfig& cfg) const {
	(void)cfg;
	return "";
}

// ALLOWED METHODS
// Check if the specified method is allowed in the configuration
bool Processor::methodIsAllowed(const Request& req,
	const LocationConfig& cfg) const
{
	const std::string& method = req.getMethod();
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
bool Processor::checkContentLength(const Request& req,
	const LocationConfig& cfg) const
{
	// TODO: compare body size with cfg.getClientMax() (0 = no limit)
	(void)req;
	(void)cfg;
	return true;
}

// Public methods --------------------------------------------------------------
Decision Processor::procRequest(const Request& req) {
	Decision	res;

	const ServerConfig* server_cfg = findConfig(req);
	if (server_cfg == NULL)
	{
		res.setStatus(INTERNAL_ERROR);
		res.setHandler(ERROR_PAGE);
		return res;
	}

	// Path escapes the root ("/../x")
	std::string target = resolvePath(req);
	if (target.empty())
	{
		res.setStatus(BAD_REQUEST);
		res.setHandler(ERROR_PAGE);
		return res;
	}

	const LocationConfig* cfg = PathUtils::findLocation(target, *server_cfg);
	if (cfg == NULL)
	{
		res.setStatus(NOT_FOUND);
		res.setHandler(ERROR_PAGE);
		return res;
	}

	// 405 must carry the Allow header, so the builder needs the methods
	if (!methodIsAllowed(req, *cfg))
	{
		res.setStatus(METHOD_NOT_ALLOWED);
		res.setHandler(ERROR_PAGE);
		const std::vector<std::string>& methods = cfg->getMethods();
		for (size_t i = 0; i < methods.size(); ++i)
			res.addAllowedMethod(methods[i]);
		return res;
	}

	if (!checkContentLength(req, *cfg))
	{
		// TODO: PAYLOAD_TOO_LARGE
	}

	// DISK PATH
	std::string disk_path = cfg->getRoot() + target;
	res.setDiskPath(disk_path);

	// TODO: stat() -> file / directory / missing (PROCESSOR_PLAN step 6-7)
	return res;
}
