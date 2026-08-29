// ServerCore.hpp
// ----------------------------------------------------------------------------

#ifndef SERVER_CORE_HPP
#define SERVER_CORE_HPP

#include <sys/epoll.h>
#include <vector>

#include "Connection.hpp"
#include "ListeningSocket.hpp"
#include "ServerConfig.hpp"

#define EVENTS_MAX 100
#define TIMEOUT_MS -1

class ServerCore {
private:
	int								epoll_fd_;
	std::vector<ServerConfig>		configs_;
	std::vector<ListeningSocket*>	socks_;
	std::map<int, Connection*>		conns_;

	ServerCore(const ServerCore &other);
	ServerCore& operator=(const ServerCore &other);

	void	initSockets();
	void	cleanSocket();
	void	cleanConnection(int fd);
	void	closeSockets();
	void	closeConnections();

	int		handleSocketEvent(int fd);
	int 	handleConnectionEvent(struct epoll_event &ev);


public:
	ServerCore(std::vector<ServerConfig> configs);
	~ServerCore();

	int	setup();
	int run();
};

#endif
