// Processor.cpp
// -----------------------------------------------------------------------------

#include <cerrno>
#include <string>
#include <vector>
#include <sstream>
#include <sys/stat.h>

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
int Processor::checkContentLength(const Request& req,
	const LocationConfig& cfg) const
{
	(void)req;
	(void)cfg;
	// TODO: compare body size with cfg.getClientMax() (0 = no limit)
	const std::string *content_len_str = req.getHeader("Content-Length");
	// Whithout content-length header the body size must be checked later
	if (content_len_str == NULL)
		return 0;

	std::stringstream stream(*content_len_str);
	size_t	content_len;

	if (!(stream >> content_len) || !stream.eof())
		return 1;
	
	if (cfg.getClientMax() > 0 && content_len > cfg.getClientMax())
		return 2;

	return 0;
}


int methodSwitch(const std::string& method) {
	if (method == "GET")
		return 1;
	if (method == "POST")
		return 2;
	if (method == "DELETE")
		return 3;
	return 0;
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
	res.setLocationConfig(cfg);

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

	switch (checkContentLength(req, *cfg)) {
		case 1: // Data error
		{
			res.setStatus(BAD_REQUEST);
			res.setHandler(ERROR_PAGE);
			return res;
		}
		case 2: // Content length greater than max body size
		{
			res.setStatus(PAYLOAD_TOO_LARGE);
			res.setHandler(ERROR_PAGE);
			return res;
		}
	}

	// DISK PATH
	std::string disk_path = cfg->getRoot() + target;
	res.setDiskPath(disk_path);

	// TODO: stat() -> file / directory / missing (PROCESSOR_PLAN step 6-7)
	struct stat stat_buf;
	if (stat(disk_path.c_str(), &stat_buf) == -1) 
	{
		switch (errno) {
			case EACCES: 
			{
				res.setStatus(FORBIDDEN);
				res.setHandler(ERROR_PAGE);
				return res;
			}
			case ENOENT: 
			{
				res.setStatus(NOT_FOUND);
				res.setHandler(ERROR_PAGE);
				return res;
			}
		}
	}
	else if (S_ISDIR(stat_buf.st_mode)) 
	{
		std::vector<std::string> indexs = cfg->getIndexs();
		// If there is a definded index file search it and read it
		if (!indexs.empty())
		{
			for (size_t i = 0; i < indexs.size(); ++i) {
				std::string index_file = disk_path + indexs[i];
				if (stat(index_file.c_str(), &stat_buf) != -1 
					&& S_ISREG(stat_buf.st_mode)) {
					res.setDiskPath(index_file);
					res.setStatus(OK);
					res.setHandler(READ_FILE);
					return res;
				}
			}
		}
		
		// If there is no index file or has not been found
		if (cfg->getAutoindex()) {
			res.setStatus(OK);
			res.setHandler(LIST_DIR);
			return res;
		} else {
			res.setStatus(FORBIDDEN);
			res.setHandler(ERROR_PAGE);
			return res;
		}
	}
	else if (S_ISREG(stat_buf.st_mode)) {
		// TODO: make a method enum list
		switch (methodSwitch(req.getMethod())) {
			case 1: // GET
			{
				res.setStatus(OK);
				res.setHandler(READ_FILE);
				return res;
			}
			case 2: // POST
			{
				res.setStatus(OK);
				res.setHandler(WRITE_FILE);
				return res;
			}
			case 3: // POST
			{
				res.setStatus(OK);
				res.setHandler(REMOVE_FILE);
				return res;
			}
		}
	}
	return res;
}
