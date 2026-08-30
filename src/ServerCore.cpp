// ServerCore.hpp
// ----------------------------------------------------------------------------

#include <cstring>
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
		log::global().critical("ServerCore: Epoll creation failed", __FILE__, __LINE__);
		// TODO: exit??
	}
}

ServerCore::~ServerCore() {
	closeConnections();
	closeSockets();
}

// Private Methods ------------------------------------------------------------
void ServerCore::initSockets() {
	log::global().debug("ServerCores: initializing server sockets", __FILE__, __LINE__);
	std::vector<ServerConfig>::iterator it;
	std::vector<ServerConfig>::iterator ite = configs_.end();
	for (it = configs_.begin(); it != ite; ++it) {
		ListeningSocket *ls = new ListeningSocket(it->getHost(), it->getListen());
		ls->setup();
		if (!ls->isReady()) {
			log::global().warning("ServerCore: socket init failed", __FILE__, __LINE__);
			continue;
		}
		socks_.push_back(ls);
		struct epoll_event	event;
		memset(&event, 0, sizeof(event));
		event.events = EPOLLIN;
		event.data.fd = ls->getFd();
		if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event.data.fd, &event) < 0) {
			log::global().warning("ServerCore: epoll ctl failed", __FILE__, __LINE__);
			continue;
		}
		std::ostringstream msg;
		msg << "ServerCore: socket added to epoll monitoring (fd: " << ls->getFd() << ")";
		log::global().debug(msg.str(), __FILE__, __LINE__);
	}
}

void ServerCore::cleanConnection(int fd) {
	std::ostringstream msg;
	msg << "ServerCore: cleaning connection (fd: " << fd << ")";
	log::global().debug(msg.str(), __FILE__, __LINE__);
	epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL);
	conns_[fd]->disconnect();
	delete conns_[fd];
	conns_.erase(fd);
}

void ServerCore::closeSockets() {
	log::global().debug("ServerCore: closing server sockets", __FILE__, __LINE__);
	std::vector<ListeningSocket*>::iterator it;
	std::vector<ListeningSocket*>::iterator ite = socks_.end();
	for (it = socks_.begin(); it != ite; ++it) {
		delete *it;
	}
}

void ServerCore::closeConnections() {
	log::global().debug("ServerCore: closing server connections", __FILE__, __LINE__);
	while (!conns_.empty()) {
		cleanConnection(conns_.begin()->first);
	}
}

int ServerCore::handleSocketEvent(int fd) {
	std::vector<ListeningSocket*>::iterator it;
	std::vector<ListeningSocket*>::iterator ite = socks_.end();
	for (it = socks_.begin(); it != ite; ++it) {
		if (fd == (*it)->getFd() ) {
			std::ostringstream msg;
			msg << "ServerCore: new socket event(fd: " << fd << ")";
			log::global().debug(msg.str(), __FILE__, __LINE__);
			ListeningSocket *ls = *it;
			int conn_fd = ls->acceptClient();
			if (conn_fd < 0) {
				log::global().warning("ServerCore: connection failed", __FILE__, __LINE__);
				continue;
			}
			struct epoll_event	event;
			memset(&event, 0, sizeof(event));
			event.events = EPOLLIN;
			event.data.fd = conn_fd;
			if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event.data.fd, &event) < 0) {
				log::global().warning("ServerCore: epoll ctl failed", __FILE__, __LINE__);
				close(conn_fd);
				continue;
			}
			conns_[conn_fd] = new Connection(conn_fd);
			
			// Connection stablished message
			std::ostringstream response;
			response << "SERVER: Connection succesfully stablished\n";
			conns_[conn_fd]->appendToWriteBuff(response.str());
			event.events = EPOLLIN | EPOLLOUT;
			epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, conn_fd, &event);
			return (fd);
		}
	}
	return -1;
}

int ServerCore::handleConnectionEvent(struct epoll_event &event) {
	int fd = event.data.fd;
	if (event.events & (EPOLLERR | EPOLLHUP)) {
		std::ostringstream msg;
		msg << "ServerCore: connecton error (fd: " << event.data.fd << ")";
		log::global().debug(msg.str(), __FILE__, __LINE__);
		cleanConnection(fd);
		return -1;
	}
	if (event.events & EPOLLIN) {
		std::ostringstream msg;
		msg << "ServerCore: connection has data to read (fd: " << fd << ")";
		log::global().debug(msg.str(), __FILE__, __LINE__);
		int read_bytes = conns_[fd]->readFromFd();
		if (read_bytes <= 0) {
			std::ostringstream msg;
			msg << "ServerCore: 0 bytes read, closing connection (fd: " << fd << ")";
			log::global().debug(msg.str(), __FILE__, __LINE__);
			cleanConnection(fd);
		}
		if (read_bytes > 0) {
			// Received bytes message
			std::ostringstream response;
			response << "SERVER: " << read_bytes << " bytes received\n";
			conns_[fd]->appendToWriteBuff(response.str());
			event.events = EPOLLIN | EPOLLOUT;
			epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &event);
		}
		return read_bytes;
	}
	if (event.events & EPOLLOUT) {
		std::ostringstream msg;
		msg << "ServerCore: connection has data to send (fd: " << fd << ")";
		log::global().debug(msg.str(), __FILE__, __LINE__);
		int fd = event.data.fd;
		int sent_bytes = conns_[fd]->writeToFd();
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
	log::global().debug("ServerCore: setup", __FILE__, __LINE__);
	initSockets();
	return 0;
}

int ServerCore::run() {
	log::global().debug("ServerCore: loop", __FILE__, __LINE__);
	struct epoll_event	events[EVENTS_MAX];
	int					event_count;
	while (true) {
		event_count = epoll_wait(epoll_fd_, events,
				EVENTS_MAX, TIMEOUT_MS);
		if (event_count < 0) {
			log::global().critical("ServerCore: epoll wait failed", __FILE__, __LINE__);
			break;
			// TODO: exit??
		}
		for (int i = 0; i < event_count; ++i) {
			int cur_fd = events[i].data.fd;
			if (conns_.count(cur_fd)) {
				handleConnectionEvent(events[i]);
			} else {
				handleSocketEvent(cur_fd);
			}
		}
	}
	return 0;
}
