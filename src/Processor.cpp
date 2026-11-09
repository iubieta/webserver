// Processor.cpp
// -----------------------------------------------------------------------------

#include <cerrno>
#include <string>
#include <vector>
#include <sstream>
#include <sys/stat.h>

#include "../inc/Processor.hpp"
#include "../inc/path_utils.hpp"
#include "../inc/log_global.hpp"

// Builders --------------------------------------------------------------------
// The builder has the server configs corresponding to the connection as input
Processor::Processor(const ServerConfig* serv_cfg):
	serv_cfg_(serv_cfg)
{}

Processor::~Processor() {}

// Private methods -------------------------------------------------------------

// Resolves the whole path to useful one
std::string Processor::resolvePath(const Request& req) const
{
	std::string new_path = PathUtils::decodePath(req.getPath());
	new_path = PathUtils::normalizePath(new_path);
	return new_path;
}

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

// Check if content-length is greater than max-body-size
int Processor::checkContentLength(const Request& req,
	const LocationConfig& cfg) const
{
	const std::string *content_len_str = req.getHeader("content-length");
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

	ft_log::global().debug("Processing request");

	Decision	res;
	res.setServerConfig(serv_cfg_);
	std::string target = resolvePath(req);
	const LocationConfig* cfg = PathUtils::findLocation(target, *serv_cfg_);
	res.setLocationConfig(cfg);
	// If a location is find fill the allowed methods in Decision
	if (cfg)
	{
		const std::vector<std::string>& methods = cfg->getMethods();
		for (size_t i = 0; i < methods.size(); ++i)
			res.addAllowedMethod(methods[i]);
	}

	// TODO: check request status

	// Path escapes the root ("/../x")
	if (target.empty())
	{
		res.setError(BAD_REQUEST);
		return res;
	}

	// Configuration searching
	if (cfg == NULL)
	{
		res.setError(NOT_FOUND);
		return res;
	}

	// If there is a return in the location process it
	if (cfg->getReturnSet()) {
		t_return ret = cfg->getReturn();
		res.setStatus(static_cast<e_status_code>(ret.code));
		res.setRedirURL(ret.destination);
		res.setHandler(REDIRECTION);
		return res;
	}

	// Searchs request method in config allowed methods
	if (!methodIsAllowed(req, *cfg))
	{
		res.setError(METHOD_NOT_ALLOWED);
		return res;
	}

	// Checks request content length -- PARSER RESPONSABILITY
	// switch (checkContentLength(req, *cfg)) {
	// 	case 1: // Data error
	// 	{
	// 		ft_log::global().error("Content length data error");
	// 		res.setError(BAD_REQUEST);
	// 		return res;
	// 	}
	// 	case 2: // Content length greater than max body size
	// 	{
	// 		ft_log::global().warning("Request content-length too long");
	// 		res.setError(PAYLOAD_TOO_LARGE);
	// 		return res;
	// 	}
	// }

	// Resolves disk path: location root + target
	std::string root = PathUtils::resolveRoot(cfg, serv_cfg_);
	std::string disk_path;
	if (!root.empty() && root[root.size() - 1] == '/' && target[0] == '/')
		 disk_path = root.substr(0, root.size() - 1) + target;
	else
		disk_path = root + target;
	res.setDiskPath(disk_path);

	// Target file handling
	struct stat stat_buf;
	// When target fails
	if (stat(disk_path.c_str(), &stat_buf) == -1)
	{
		switch (errno) {
			case EACCES: // Permission error
			{
				res.setError(FORBIDDEN);
				return res;
			}
			case ENOTDIR:
				// FALLTHROUGH
			case ENOENT : // File not found
			{
				// Post method -> Create file
				if (req.getMethod() == "POST") {
					res.setHandler(WRITE_FILE);
					return res;
				}

				// If GET or DELETE methods -> NOT FOUND
				res.setError(NOT_FOUND);
				return res;
			}
			default:
			{
				res.setError(INTERNAL_ERROR);
				return res;
			}
		}
	}
	// When target is a dir: checks for index or autoindex
	else if (S_ISDIR(stat_buf.st_mode))
	{
		switch (methodSwitch(req.getMethod())) {
			case 1: // GET
			{
				// If there is a definded index file search it and read it
				std::vector<std::string> indexs = cfg->getIndexs();
				if (!indexs.empty())
				{
					for (size_t i = 0; i < indexs.size(); ++i) {
						std::string index_file = disk_path + indexs[i];
						if (stat(index_file.c_str(), &stat_buf) != -1
							&& S_ISREG(stat_buf.st_mode)) {
							res.setDiskPath(index_file);
							res.setHandler(READ_FILE);
							return res;
						}
					}
				}

				// If there is no index file tries autoindex
				if (cfg->getAutoindex()) {
					res.setHandler(LIST_DIR);
					return res;

				// If autoindex is off return 403
				} else {
					res.setError(FORBIDDEN);
					return res;
				}
			}
			case 2: // POST
			{
				res.setError(FORBIDDEN);
				return res;
			}
			case 3: // DELETE
			{
				res.setHandler(REMOVE_FILE);
				return res;
			}
			default:
			{
				res.setError(METHOD_NOT_ALLOWED);
				return res;
			}
		}
	}
	// When target is an accesible file
	else if (S_ISREG(stat_buf.st_mode)) {
		switch (methodSwitch(req.getMethod())) {
			case 1: // GET
			{
				res.setHandler(READ_FILE);
				return res;
			}
			case 2: // POST
			{
				res.setHandler(WRITE_FILE);
				return res;
			}
			case 3: // DELETE
			{
				res.setHandler(REMOVE_FILE);
				return res;
			}
			default:
			{
				res.setError(METHOD_NOT_ALLOWED);
				return res;
			}
		}
	}
	// Target is neither a directory nor a regular file (fifo, socket...)
	else
	{
		res.setError(FORBIDDEN);
		return res;
	}
	return res;
}
