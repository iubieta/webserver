#ifndef SERVER_CONFIG_HPP
#define SERVER_CONFIG_HPP

#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <cstring>
#include <string>


#include "LocationConfig.hpp"

class ServerConfig 
{
    private:
        unsigned int                 listen_;
        unsigned int                 host_;
        std::string                  server_name_;
        std::string                  root_;
        size_t                       client_max_body_size_;
        bool                         autoindex_;
        std::vector<std::string>     index_;
        std::vector<LocationConfig>  locations_;

    public:
        ServerConfig();
        ServerConfig(const ServerConfig &other);
        ServerConfig &operator=(const ServerConfig &other);
        ~ServerConfig();
		
		unsigned int getListen() const;
        unsigned int getHost() const;
        size_t getClientMaxBody() const;
        bool getAutoindex() const;
        const std::string &getServerName() const;
        const std::string &getRoot() const;
        const std::vector<std::string> &getIndexs() const;
        const std::vector<LocationConfig> &getLocations() const;
        const std::string &getIndex(size_t index) const;
        const LocationConfig &getLocation(size_t) const;

        int setListen(const std::string &listen);
        int setHost(const std::string &host);
        int setClientMax(const std::string &client_max);
        void setAutoindex(bool autoindex);
        int setServerName(const std::string &server_name);
        int setRoot(const std::string &root);

        void addIndex(const std::string &index_file);
        void addLocation(const LocationConfig &location);
};

#endif