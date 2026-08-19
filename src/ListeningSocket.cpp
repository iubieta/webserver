// ListeningSocket.cpp
// ----------------------------------------------------------------------------

#include <cstring>
#include <sstream>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#include "../inc/ListeningSocket.hpp"
#include "../inc/log_global.hpp"
#include "../inc/Logger.hpp"

// Constructors ---------------------------------------------------------------
ListeningSocket::ListeningSocket(const std::string &host, int port) :
	host_(host), port_(port), fd_(-1)
{
	if (port < 1 || port > 65535)
		log::global().warning(
				"invalid port passed to the constructor", __FILE__,__LINE__);
	log::global().debug("ListeningSocket instance created", __FILE__, __LINE__);
}

ListeningSocket::~ListeningSocket() {
	if (fd_ >= 0) {
		close(fd_);
		log::global().debug("fd closed", __FILE__, __LINE__);
	}
}

// Getters --------------------------------------------------------------------
const std::string	&ListeningSocket::getHost() const {
	return host_;
}

int		ListeningSocket::getPort() const {
	return port_;
}

int		ListeningSocket::getFd() const {
	return fd_;
}

// Public methods -------------------------------------------------------------
bool	ListeningSocket::setup() {
	int status;
	struct addrinfo	hints;
	struct addrinfo	*serv_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	std::ostringstream oss;
	oss << port_;
	std::string port_str = oss.str();

	status = getaddrinfo(host_.c_str(), port_str.c_str(), &hints, &serv_info);
	if (status != 0) {
		std::string error_msg = "gai error: " + std::string(gai_strerror(status));
		log::global().error(error_msg, __FILE__, __LINE__);
		return false;
	}
	
	int	sock_fd = -1;
	struct addrinfo *cur_ai = serv_info;
	bool bound = false;
	while (cur_ai) {
		sock_fd = socket(cur_ai->ai_family, cur_ai->ai_socktype, 
				cur_ai->ai_protocol);
		if (sock_fd == -1) {
			cur_ai = cur_ai->ai_next;
			log::global().debug("socket creation failed", __FILE__, __LINE__);
			continue;
		}
		
		int yes = 1;
		if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
			cur_ai = cur_ai->ai_next;
			close(sock_fd);
			log::global().debug("set socket options failed", __FILE__, __LINE__);
			continue;
		}

		if (fcntl(sock_fd, F_SETFL, O_NONBLOCK) < 0) {
			cur_ai = cur_ai->ai_next;
			close(sock_fd);
			log::global().debug("set socket as nonblocking failed", __FILE__, __LINE__);
			continue;
		}

		if (bind(sock_fd, cur_ai->ai_addr, cur_ai->ai_addrlen) == 0) {
			bound = true;
			log::global().debug("socket bound succesfully", __FILE__, __LINE__);
			break;
		} else {
			cur_ai = cur_ai->ai_next;
			close(sock_fd);
			log::global().debug("socket binding failed", __FILE__, __LINE__);
			continue;
		}
	}
	freeaddrinfo(serv_info);
	if (!bound) {
		log::global().warning("no valid address found", __FILE__, __LINE__);
		return false;
	}
	if (listen(sock_fd, BACKLOG) == 0) {
		fd_ = sock_fd;
		log::global().debug("socket listening", __FILE__, __LINE__);
		return true;
	} else {
		close(sock_fd);
		log::global().warning("socket failed to start listening", __FILE__, __LINE__);
		return false;
	}
}

bool	ListeningSocket::isReady() const {
	return fd_ >= 0;
}

int		ListeningSocket::acceptClient() const {
	struct sockaddr_storage	client_addr;
	socklen_t addr_size = sizeof client_addr;
	return accept(fd_, (struct sockaddr *)&client_addr, &addr_size);
}
