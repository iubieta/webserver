// ListeningSocket.cpp
// ----------------------------------------------------------------------------

#include <cstring>
#include <sstream>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#include "../inc/ListeningSocket.hpp"
#include "../inc/log_macros.hpp"

// Constructors ---------------------------------------------------------------
ListeningSocket::ListeningSocket(unsigned int host, unsigned int port) :
	host_(host), port_(port), fd_(-1)
{
	if (port < 1 || port > 65535) {
		LOG_ERROR("ListeningSocket: invalid port " << port);
		throw std::invalid_argument("Port out of range 1-65535");
	}
}

ListeningSocket::~ListeningSocket() {
	if (fd_ >= 0) {
		close(fd_);
		LOG_DEBUG("ListeningSocket: socket fd closed -> "
			<< ", port: " << port_
			<< "fd: " << fd_
		);
	}
}

// Getters --------------------------------------------------------------------
unsigned int	ListeningSocket::getHost() const {
	return host_;
}

unsigned int	ListeningSocket::getPort() const {
	return port_;
}

int		ListeningSocket::getFd() const {
	return fd_;
}

// Public methods -------------------------------------------------------------
bool	ListeningSocket::setup() {
	struct sockaddr_in	addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_);
	addr.sin_addr.s_addr = host_;

	int	sock_fd = -1;
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1) {
		LOG_DEBUG("ListeningSocket: socket creation failed");
		return false;
	}
	LOG_DEBUG("ListeningSocket: socket created -> fd: " << sock_fd);
	
	int yes = 1;
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		LOG_DEBUG("ListeningSocket: set socket options failed");
		close(sock_fd);
		return false;
	}

	if (fcntl(sock_fd, F_SETFL, O_NONBLOCK) < 0) {
		LOG_DEBUG("ListeningSocket: setting socket fd as nonblocking failed");
		close(sock_fd);
		return false;
	}

	if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
		LOG_DEBUG("ListeningSocket: socket bound succesfully");
	} else {
		LOG_DEBUG("ListeningSocket: socket binding failed");
		close(sock_fd);
		return false;
	}

	if (listen(sock_fd, BACKLOG) == 0) {
		fd_ = sock_fd;
		LOG_DEBUG("ListeningSocket: socket listening");
		return true;
	} else {
		LOG_WARNING("ListeningSocket: socket failed to start listening");
		close(sock_fd);
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
