// ServerCore.hpp
// ----------------------------------------------------------------------------

#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <sstream>

#include "../inc/ServerCore.hpp"
#include "../inc/log_global.hpp"
#include "../inc/Logger.hpp"

// Constructors ---------------------------------------------------------------
ServerCore::ServerCore(std::vector<ServerConfig> configs) : configs_(configs) {
	epoll_fd_ = epoll_create(1);
	if (epoll_fd_ < 0) {
		ft_log::global().critical("ServerCore: Epoll creation failed", __FILE__, __LINE__);
		// TODO: exit??
	}
}

ServerCore::~ServerCore() {
	closeConnections();
	closeSockets();
	close(epoll_fd_);
}

// Private Methods ------------------------------------------------------------

// Socket init based on server config
void ServerCore::initSockets() {
	ft_log::global().debug("ServerCores: initializing server sockets", __FILE__, __LINE__);

	std::vector<ServerConfig>::iterator it;
	std::vector<ServerConfig>::iterator ite = configs_.end();
	// For each config init a socket
	for (it = configs_.begin(); it != ite; ++it) {
		ListeningSocket *ls = new ListeningSocket(it->getHost(), it->getListen());
		// If socket setup fails, warn and delete the pointer
		if (!ls->setup()) {
			ft_log::global().warning("ServerCore: socket init failed", __FILE__, __LINE__);
			delete ls;
			continue;
		}
		// Add the socket to epoll monitoring
		struct epoll_event	event;
		memset(&event, 0, sizeof(event));
		event.events = EPOLLIN;
		event.data.fd = ls->getFd();
		if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event.data.fd, &event) < 0) {
			ft_log::global().warning("ServerCore: epoll ctl failed", __FILE__, __LINE__);
			continue;
		}
		// Add the socket to server socket vector
		socks_.push_back(ls);
		std::ostringstream msg;
		msg << "ServerCore: socket added to epoll monitoring (fd: " << ls->getFd() << ")";
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
	}
}

// Connection cleaning 
void ServerCore::cleanConnection(int fd) {
	std::ostringstream msg;
	msg << "ServerCore: cleaning connection (fd: " << fd << ")";
	ft_log::global().debug(msg.str(), __FILE__, __LINE__);

	// Delete the connection fd from epoll monitoring
	if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL) < 0) {
		msg.str() = "";
		msg << "ServerCore: epoll_ctl failed" << fd << ")";
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
	}
	
	// Clean the connection
	conns_[fd]->disconnect();
	delete conns_[fd];
	conns_.erase(fd);
}

void ServerCore::closeSockets() {
	ft_log::global().debug("ServerCore: closing server sockets", __FILE__, __LINE__);
	std::vector<ListeningSocket*>::iterator it;
	std::vector<ListeningSocket*>::iterator ite = socks_.end();
	for (it = socks_.begin(); it != ite; ++it) {
		// ListeningSocket destructor closes the fd
		delete *it;
	}
}

void ServerCore::closeConnections() {
	ft_log::global().debug("ServerCore: closing server connections", __FILE__, __LINE__);
	while (!conns_.empty()) {
		// Cleans alway first element, no pointer update needed
		cleanConnection(conns_.begin()->first);
	}
}

int ServerCore::handleSocketEvent(int fd) {
	std::vector<ListeningSocket*>::iterator it;
	std::vector<ListeningSocket*>::iterator ite = socks_.end();
	for (it = socks_.begin(); it != ite; ++it) {
		// Search the socket of the event
		if (fd == (*it)->getFd() ) {
			std::ostringstream msg;
			msg << "ServerCore: new socket event(fd: " << fd << ")";
			ft_log::global().debug(msg.str(), __FILE__, __LINE__);
			
			ListeningSocket *ls = *it;

			// accepts the connection
			int conn_fd = ls->acceptClient();
			if (conn_fd < 0) {
				ft_log::global().warning("ServerCore: connection failed", __FILE__, __LINE__);
				continue;
			}

			// Sets connection socket as nonblocking
			if (fcntl(conn_fd, F_SETFL, O_NONBLOCK) < 0) {
				ft_log::global().debug("ServerCore: set connection as nonblocking failed", __FILE__, __LINE__);
				close(conn_fd);
				continue;
			}

			// Adds connection to epoll monitoring
			struct epoll_event	event;
			memset(&event, 0, sizeof(event));
			event.events = EPOLLIN;
			event.data.fd = conn_fd;
			if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event.data.fd, &event) < 0) {
				ft_log::global().warning("ServerCore: epoll ctl failed", __FILE__, __LINE__);
				close(conn_fd);
				continue;
			}
			// Add connection to the server connection map
			conns_[conn_fd] = new Connection(conn_fd);
			
			// DEBUG: Connection stablished message sending
			std::ostringstream response;
			response << "SERVER: Connection succesfully stablished\n";
			conns_[conn_fd]->appendToWrite(response.str());
			event.events = EPOLLIN | EPOLLOUT;
			if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, conn_fd, &event) < 0) {
				ft_log::global().warning("ServerCore: epoll ctl failed", __FILE__, __LINE__);
			}

			return (fd);
		}
	}
	return -1;
}

// Connections events handler
int ServerCore::handleConnectionEvent(struct epoll_event &event) {
	int fd = event.data.fd;

	// Connection errors
	if (event.events & (EPOLLERR | EPOLLHUP)) {
		std::ostringstream msg;
		msg << "ServerCore: connecton error (fd: " << event.data.fd << ")";
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
		cleanConnection(fd);
		return -1;
	}

	// Inputs
	if (event.events & EPOLLIN) {
		std::ostringstream msg;
		msg << "ServerCore: connection has data to read (fd: " << fd << ")";
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
		
		// Reads the data
		int read_bytes = conns_[fd]->readFromFd();
		// Error or client disconnected
		if (read_bytes <= 0) {
			std::ostringstream msg;
			msg << "ServerCore: 0 bytes read, closing connection (fd: " << fd << ")";
			ft_log::global().debug(msg.str(), __FILE__, __LINE__);
			cleanConnection(fd);
		}
		// Data received
		if (read_bytes > 0) {
			// DEBUG: received bytes message
			std::ostringstream response;
			response << "SERVER: " << read_bytes << " bytes received\n";
			conns_[fd]->appendToWrite(response.str());
			event.events = EPOLLIN | EPOLLOUT;
			epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
		}
		return read_bytes;
	}

	// Outputs
	if (event.events & EPOLLOUT) {
		std::ostringstream msg;
		msg << "ServerCore: connection has data to send (fd: " << fd << ")";
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
		// Sending data
		int fd = event.data.fd;
		int sent_bytes = conns_[fd]->writeToFd();
		// If buffer is empty after sending uncheck the output flag
		if (!conns_[fd]->wantsWrite()) {
			event.events = EPOLLIN;
			epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
		}
		return sent_bytes;
	}
	return 0;
}

// Public Methods -------------------------------------------------------------
int ServerCore::setup() {
	ft_log::global().debug("ServerCore: setup", __FILE__, __LINE__);
	initSockets();
	return 0;
}

int ServerCore::run() {
	ft_log::global().debug("ServerCore: loop", __FILE__, __LINE__);
	struct epoll_event	events[EVENTS_MAX];
	int					event_count;

	// Server loop
	while (true) {
		event_count = epoll_wait(epoll_fd_, events,
				EVENTS_MAX, TIMEOUT_MS);
		// If epoll wait fails close the server
		if (event_count < 0) {
			ft_log::global().critical("ServerCore: epoll wait failed", __FILE__, __LINE__);
			return -1;
		}
		// Handle events
		for (int i = 0; i < event_count; ++i) {
			int cur_fd = events[i].data.fd;
			// If the fd is in connection map
			if (conns_.count(cur_fd)) {
				handleConnectionEvent(events[i]);
			} else {
				handleSocketEvent(cur_fd);
			}
		}
	}
	return 0;
}
