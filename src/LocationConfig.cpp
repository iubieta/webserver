#include "../inc/LocationConfig.hpp"
#include <sstream>
#include <limits>

LocationConfig::LocationConfig()
			: root_(""),
			  autoindex_(false),
			  allow_methods_(),
			  index_(),
			  client_max_body_size_(0)
{}

LocationConfig::LocationConfig(const LocationConfig &other)
			: root_(other.root_),
			  autoindex_(other.autoindex_),
			  allow_methods_(other.allow_methods_),
			  index_(other.index_),
			  client_max_body_size_(other.client_max_body_size_)
{}

LocationConfig &LocationConfig::operator=(const LocationConfig &other)
{
	if(this != &other)
	{
		root_ = other.root_;
		autoindex_ = other.autoindex_;
		allow_methods_ = other.allow_methods_;
		index_ = other.index_;
		client_max_body_size_ = other.client_max_body_size_;
	}
	return *this;
}

LocationConfig::~LocationConfig() {}

const std::string &LocationConfig::getRoot() const
{
	return root_;
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

int LocationConfig::setRoot(const std::string root)
{
	if(root.empty())
		return -1;
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