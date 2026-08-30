// ListeningSocket.cpp
// ----------------------------------------------------------------------------

#include <cstring>
#include <sstream>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#include "../inc/ListeningSocket.hpp"
#include "../inc/log_global.hpp"
#include "../inc/Logger.hpp"

// Constructors ---------------------------------------------------------------
ListeningSocket::ListeningSocket(unsigned int host, unsigned int port) :
	host_(host), port_(port), fd_(-1)
{
	if (port < 1 || port > 65535) {
		std::ostringstream msg;
		msg << "ListeningSocket: invalid port " << port;
		log::global().warning(msg.str(), __FILE__,__LINE__);
	}
	std::ostringstream msg;
	msg << "ListeningSocket: obj created (host: " 
		<< host << ", port: " << port << ")";
	log::global().debug(msg.str(), __FILE__, __LINE__);
}

ListeningSocket::~ListeningSocket() {
	if (fd_ >= 0) {
		close(fd_);
		std::ostringstream msg;
		msg << "ListeningSocket: fd " << fd_ << "closed";
		log::global().debug(msg.str(), __FILE__, __LINE__);
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
	int status;
	struct sockaddr_in	addr;

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_);
	addr.sin_addr.s_addr = host_;

	int	sock_fd = -1;
	bool bound = false;
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1) {
		log::global().debug("ListeningSocket: socket creation failed", __FILE__, __LINE__);
		return false;
	}
	
	int yes = 1;
	if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
		log::global().debug("ListeningSocket: set socket options failed", __FILE__, __LINE__);
		close(sock_fd);
		return false;
	}

	if (fcntl(sock_fd, F_SETFL, O_NONBLOCK) < 0) {
		log::global().debug("ListeningSocket: set socket as nonblocking failed", __FILE__, __LINE__);
		close(sock_fd);
		return false;
	}

	if (bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
		log::global().debug("ListeningSocket: socket bound succesfully", __FILE__, __LINE__);
	} else {
		log::global().debug("ListeningSocket: socket binding failed", __FILE__, __LINE__);
		close(sock_fd);
		return false;
	}

	if (listen(sock_fd, BACKLOG) == 0) {
		fd_ = sock_fd;
		std::ostringstream msg;
		msg << "ListeningSocket: socket listening (fd: " << fd_ << ")";
		log::global().debug(msg.str(), __FILE__, __LINE__);
		return true;
	} else {
		log::global().warning("ListeningSocket: socket failed to start listening", __FILE__, __LINE__);
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
