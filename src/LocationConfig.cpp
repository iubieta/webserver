#include "../inc/LocationConfig.hpp"
#include <sstream>
#include <iostream>
#include <limits>
#include <strings.h>

LocationConfig::LocationConfig()
			: target_(""), 
			  root_(""),
			  autoindex_(false),
			  allow_methods_(),
			  index_(),
			  client_max_body_size_(0),
			  alias_(""),
			  root_set_(false),
			  alias_set_(false),
			  autoindex_set_(false),
			  client_max_body_set_(false),
			  return_(),
			  return_set_(false),
			  error_page_()

{}

LocationConfig::LocationConfig(const LocationConfig &other)
			: target_(other.target_), 
			  root_(other.root_),
			  autoindex_(other.autoindex_),
			  allow_methods_(other.allow_methods_),
			  index_(other.index_),
			  client_max_body_size_(other.client_max_body_size_),
			  alias_(other.alias_),
			  root_set_(other.root_set_),
			  alias_set_(other.alias_set_),
			  autoindex_set_(other.autoindex_set_),
			  client_max_body_set_(other.client_max_body_set_),
			  return_(other.return_),
			  return_set_(other.return_set_),
			  error_page_(other.error_page_)
{}

LocationConfig &LocationConfig::operator=(const LocationConfig &other)
{
	if(this != &other)
	{
		target_ = other.target_;
		root_ = other.root_;
		autoindex_ = other.autoindex_;
		allow_methods_ = other.allow_methods_;
		index_ = other.index_;
		client_max_body_size_ = other.client_max_body_size_;
		alias_ = other.alias_;
		root_set_ = other.root_set_;
		alias_set_ = other.alias_set_;
		autoindex_set_ = other.autoindex_set_;
		client_max_body_set_ = other.client_max_body_set_;
		return_  = other.return_;
		return_set_ = other.return_set_;
		error_page_ = other.error_page_;
	}
	return *this;
}

LocationConfig::~LocationConfig() {}

const std::string &LocationConfig::getRoot() const
{
	return root_;
}

// Operators
bool LocationConfig::operator==(const LocationConfig &other) const
{
	if (target_ == other.target_
		&& root_ == other.root_
		&& autoindex_ == other.autoindex_
		&& allow_methods_ == other.allow_methods_
		&& index_ == other.index_
		&& client_max_body_size_ == other.client_max_body_size_
		&& alias_ == other.alias_
		&& root_set_ == other.root_set_
		&& autoindex_set_ == other.autoindex_set_
		&& client_max_body_set_ == other.client_max_body_set_
		&& alias_set_ == other.alias_set_
		&& return_ == other.return_
		&& return_set_ == other.return_set_
		&& error_page_ == other.error_page_)
		return true;
	return false;
}
bool LocationConfig::getAutoindex() const
{
	return autoindex_;
}

size_t LocationConfig::getClientMax() const
{
	return client_max_body_size_;
}

const std::vector<std::string> &LocationConfig::getIndexs() const
{
	return index_;
}

const std::vector<std::string> &LocationConfig::getMethods() const
{
	return allow_methods_;
}

const std::string &LocationConfig::getIndex(size_t index) const
{
	return index_.at(index);
}

const std::string &LocationConfig::getMethod(size_t index) const
{
	return allow_methods_.at(index);
}

const std::string &LocationConfig::getTarget() const
{
	return target_;
}

const std::string &LocationConfig::getAlias() const
{
	return alias_;
}

int LocationConfig::setRoot(const std::string &root)
{
	if(root.empty())
		return -1;
	if(root[0] != '/')
	{
		std::cerr << "root is not an absolute path\n";
		return -1;
	}
		
	root_ = root;
	return 0;
}

int LocationConfig::setClientMax(const std::string &client_max_str)
{
	std::stringstream ss(client_max_str);
	unsigned long long max_body;
	char leftover;

	if (client_max_str.empty())
		return -1;
	
	if (client_max_str[0] == '-')
        return -1;

	if(!(ss >> max_body) || (ss >> leftover))
		return -1;

	if (max_body > std::numeric_limits<size_t>::max())
		return -1;

	client_max_body_size_= static_cast<size_t>(max_body);

	return 0;
}

int LocationConfig::setClientMax(size_t clientMax)
{
	client_max_body_size_ = clientMax;
	return 0;
}

int	LocationConfig::toRedirectStatusCode(int number, e_status_code &status)
{
	switch (number)
	{
		case MOVED_PERMANENTLY:
		case FOUND:
		case TEMPORARY_REDIR:
		case PERMANENT_REDIR:
			status = static_cast<e_status_code>(number);
			return (0);
		default:
			return (-1);
	}
}

int	LocationConfig::toErrorStatusCode(int number, e_status_code &status)
{
	switch (number)
	{
		case BAD_REQUEST:
		case FORBIDDEN:
		case NOT_FOUND:
		case METHOD_NOT_ALLOWED:
		case REQUEST_TIMEOUT:
		case LENGTH_REQUIRED:
		case PAYLOAD_TOO_LARGE:
		case URI_TOO_LONG:
		case UNSUPPORTED_MEDIA_TYPE:
		case HEADERS_TOO_LARGE:
		case INTERNAL_ERROR:
		case NOT_IMPLEMENTED:
		case BAD_GATEWAY:
		case SERVICE_UNAVAILABLE:
		case VERSION_NOT_SUPPORTED:
			status = static_cast<e_status_code>(number);
			return (0);
		default:
			return (-1);
	}
}

int	LocationConfig::setReturn(const std::string &code,
		const std::string &destination)
{
	std::stringstream	ss(code);
	int					code_num;
	char				leftover;
	e_status_code		status;

	if (code.empty() || destination.empty())
		return (-1);
	if (!(ss >> code_num) || (ss >> leftover))
		return (-1);
	if (toRedirectStatusCode(code_num, status) == -1)
		return -1;
	if (destination[0] != '/'
		&& destination.find("http://") != 0
		&& destination.find("https://") != 0)
		return (-1);
	return_.code = status;
	return_.destination = destination;
	return (0);
}

//This function needs to be validated.
// int LocationConfig::setReturn(e_status_code code, const std::string &destination)
// {
// 	return_.code = code;
// 	return_.destination = destination;
// 	return 0;
// }

void LocationConfig::setReturnSet(bool returnSet)
{
	return_set_ = returnSet;
}

void LocationConfig::addIndex(const std::string &index)
{
	if(!index.empty())
		index_.push_back(index);
}

int LocationConfig::addMethod(const std::string &method)
{
	if (method != "GET" && method != "POST" && method != "DELETE")
    	return -1;
    
	allow_methods_.push_back(method);
    return 0;
}

int LocationConfig::setVecIndex(const std::vector<std::string> &index)
{
	if (index.empty())
		return -1;
	index_ = index;
	return 0;
}

int LocationConfig::setAutoindex(const std::string &autoindex)
{
	if(autoindex.empty())
		return -1;
	else if (autoindex == "on")
	{
		autoindex_ = true;
		return 0;
	}
	else if (autoindex == "off")
	{
		autoindex_ = false;
		return 0;
	}
	return -1;
}

int LocationConfig::setAutoindex(bool autoindex)
{
	autoindex_ = autoindex;
	return 0;
}

int LocationConfig::setTarget(const std::string &target)
{
	if (target.empty())
		return -1;
	if (target[0] != '/')
		return -1;
	target_ = target;
	return 0;
}

int LocationConfig::setAlias(const std::string &alias)
{
	if (alias.empty())
		return -1;
	if (alias[0] != '/')
		return -1;
	if (alias[alias.size() - 1] != '/')
		return -1;
	alias_ = alias;
	return 0;
}

bool LocationConfig::getRootSet() const
{
	return root_set_;
}

void LocationConfig::setRootSet(bool rootSet)
{
	root_set_ = rootSet;
}

bool LocationConfig::getAliasSet() const
{
	return alias_set_; 
}

void LocationConfig::setAliasSet(bool aliasSet)
{
	alias_set_ = aliasSet;
}

bool LocationConfig::getAutoindexSet() const
{
	return autoindex_set_;
}

void LocationConfig::setAutoindexSet(bool autoindexSet)
{
	autoindex_set_ = autoindexSet;
}

bool LocationConfig::getClientMaxbodySet() const
{
	return client_max_body_set_;
}

const t_return &LocationConfig::getReturn() const
{
	return return_;
}

bool LocationConfig::getReturnSet() const
{
	return return_set_;
}

void LocationConfig::setClientMaxBodySet(bool clientMaxbodySet)
{
	client_max_body_set_ = clientMaxbodySet;
}

const std::string *LocationConfig::getErrorPage(e_status_code key) const
{
	std::map<e_status_code, std::string>::const_iterator it = error_page_.find(key);

	if (it != error_page_.end())	
		return &(it->second);
	return NULL;
}

const std::map<e_status_code, std::string> &LocationConfig::getErrorPages() const
{
	return error_page_;
}

int LocationConfig::addErrorPage(const std::string &code, const std::string &page)
{
    std::stringstream ss(code);
    char letfover;
    int code_;
	e_status_code status;

    if (code.empty() || page.empty())
        return -1;
    if(code[0] == '+' || code.size() != 3)
        return -1;
    if (!(ss >> code_) || (ss >> letfover))
        return -1;
    if(toErrorStatusCode(code_, status) == -1)
		return -1;
	
    if (page[0] != '/')
        return -1;
    
    if (error_page_.count(status) != 0)
	{
		std::cerr<<"error page: repeated field\n";
        return (-1);
	}
	error_page_.insert(std::make_pair(status, page));
	return (0);
}

void LocationConfig::setErrorPages(const std::map<e_status_code, std::string> &error_pages)
{
	error_page_ = error_pages;
}