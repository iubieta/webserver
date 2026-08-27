#include "../inc/ServerConfig.hpp"
#include <sstream>
#include <limits>

ServerConfig::ServerConfig()
    : listen_(80), 
      host_(0x7F000001),
      server_name_("localhost"), 
      root_(""), 
      client_max_body_size_(0),
      autoindex_(false),
      index_(),
      locations_()
{}


ServerConfig::ServerConfig(const ServerConfig &other)
    : listen_(other.listen_), 
      host_(other.host_), 
      server_name_(other.server_name_), 
      root_(other.root_), 
      client_max_body_size_(other.client_max_body_size_),
      autoindex_(other.autoindex_), 
      index_(other.index_), 
      locations_(other.locations_) 
{}


ServerConfig &ServerConfig::operator=(const ServerConfig &other)
{
    if (this != &other)
    {
        listen_ = other.listen_;
        host_ = other.host_;
        server_name_ = other.server_name_;
        root_ = other.root_;
        client_max_body_size_ = other.client_max_body_size_;
        autoindex_ = other.autoindex_;
        index_ = other.index_;
        locations_ = other.locations_;
    }
    return *this;
}

ServerConfig::~ServerConfig() {}

unsigned int ServerConfig::getListen() const
{
	return listen_;
}

unsigned int ServerConfig::getHost() const
{
	return host_;
}

size_t ServerConfig::getClientMaxBody() const
{
	return client_max_body_size_;
}

bool ServerConfig::getAutoindex() const
{
	return autoindex_;
}

const std::string &ServerConfig::getServerName() const
{
	return server_name_;
}

const std::string &ServerConfig::getRoot() const
{
	return root_;
}

const std::vector<std::string> &ServerConfig::getIndexs() const
{
	return index_;
}

const std::vector<LocationConfig> &ServerConfig::getLocations() const
{
	return locations_;
}

const std::string &ServerConfig::getIndex(size_t index) const
{
	return index_.at(index);
}

const LocationConfig &ServerConfig::getLocation(size_t index) const
{
	return locations_.at(index);
}

int ServerConfig::setListen(const std::string &listen_str)
{
	long listen;
	std::stringstream ss(listen_str);
	char leftover;

	if (listen_str.empty())
		return -1;

	if(!(ss >> listen) || ss >> leftover)
		return -1;

	if (listen < 1 || listen > 65535)
        return -1;

    listen_ = static_cast<unsigned int>(listen);
    return 0;
}

int ServerConfig::setHost(const std::string& host) 
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    struct sockaddr_in *ipv4;
    int status;

	if (host.empty())
		return -1;

    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // only IPv4
    hints.ai_socktype = SOCK_STREAM; // Sockets TCP

    // getaddrinfo converts a string (e.g., “127.0.0.1” or “localhost”) 
    //into a network structure
    status = getaddrinfo(host.c_str(), NULL, &hints, &res);
    if (status != 0 || res == NULL) 
    	return -1; 
    
	// We extract the 32-bit integer from the res structure
    ipv4 = reinterpret_cast<struct sockaddr_in *>(res->ai_addr);
    this->host_ = ipv4->sin_addr.s_addr;

    // It is ESSENTIAL to free the memory allocated by getaddrinfo
    freeaddrinfo(res);

    return 0;
}

int ServerConfig::setClientMax(const std::string &client_max_str)
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

int ServerConfig::setAutoindex(const std::string &autoindex)
{
    if (autoindex.empty())
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

int ServerConfig::setServerName(const std::string &server_name)
{
	if(server_name.empty())
		return -1;
	server_name_ = server_name;

	return 0;
}

int ServerConfig::setRoot(const std::string &root)
{
	if (root.empty())
		return -1;
	root_ = root;
	return 0;
}

void ServerConfig::addIndex(const std::string &index)
{
	if(!index.empty())
		index_.push_back(index);
}

void ServerConfig::addLocation(const LocationConfig &location)
{
	locations_.push_back(location);
}