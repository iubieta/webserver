// ServerCore.hpp
// ----------------------------------------------------------------------------

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
#include <sstream>

#include "../inc/ServerCore.hpp"
#include "../inc/log_macros.hpp"

// Constructors ---------------------------------------------------------------
ServerCore::ServerCore(std::vector<ServerConfig> configs) : configs_(configs) {
	epoll_fd_ = epoll_create(1);
	if (epoll_fd_ < 0) {
		throw std::runtime_error("epoll creation failed");
	}
}

ServerCore::~ServerCore() {
	closeConnections();
	closeSockets();
	close(epoll_fd_);
}

// Private Methods ------------------------------------------------------------

// Event handlers
int ServerCore::createEvent(int fd, uint32_t mask) {
	struct epoll_event	event = {};
	event.events = mask;
	event.data.fd = fd;
	return epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event.data.fd, &event);
}

int ServerCore::modifyEvent(int fd, uint32_t mask) {
	struct epoll_event	event = {};
	event.events = mask;
	event.data.fd = fd;
	return epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event.data.fd, &event);
}

int ServerCore::deleteEvent(int fd) {
	return epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, NULL);
}

// Socket init based on server config
void ServerCore::initSockets() {
	LOG_INFO("ServerCore: initializing server sockets");

	std::vector<ServerConfig>::iterator it;
	std::vector<ServerConfig>::iterator ite = configs_.end();
	// For each config init a socket
	for (it = configs_.begin(); it != ite; ++it) {

		// Create new socket
		ListeningSocket *ls = new ListeningSocket(it->getHost(), it->getListen());
		// If socket setup fails, warn and delete the pointer
		if (!ls->setup()) {
			LOG_WARNING("ServerCore: socket init failed");
			delete ls;
			continue;
		}
		
		// Add the socket to epoll monitoring
		struct epoll_event	event;
		memset(&event, 0, sizeof(event));
		event.events = EPOLLIN;
		event.data.fd = ls->getFd();
		// If epoll adding fails delete the socket
		if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event.data.fd, &event) < 0) {
			int err = errno;
			LOG_WARNING("ServerCore: epoll_ctl failed, " 
				<< "fd: " << ls->getFd()
				<< ", error: " << strerror(err)
			);
			delete ls;
			continue;
		}
		
		// Add the socket to listening socket map
		socks_.insert(
			std::pair<int, Listener>(ls->getFd(), Listener(ls, &(*it)))
		);
		LOG_INFO("ServerCore: socket initiated -> fd: " << ls->getFd());
	}
}

// Connection cleaning 
void ServerCore::cleanConnection(int fd)
{
	std::map<int, Connection*>::iterator	it;
	std::ostringstream					msg;
	int									saved_error;

	LOG_INFO("ServerCore: cleaning connection -> fd: " << fd);
	it = conns_.find(fd);
	if (it == conns_.end())
	{
		LOG_WARNING("ServerCore: fd not found in connection map");
		return;
	}

	if (deleteEvent(fd) < 0)
	{
		saved_error = errno;
		LOG_WARNING("ServerCore: epoll deletion failed, " << strerror(saved_error));
	}

	// Aunque falle la retirada de epoll, intentamos liberar el cliente.
	it->second->disconnect();
	delete it->second;
	conns_.erase(it);
	LOG_INFO("ServerCore: connection cleaned");
}

void ServerCore::closeSockets() {
	LOG_DEBUG("ServerCore: closing sockets");
	while (!socks_.empty()) {
		delete socks_.begin()->second.sock_;
		socks_.erase(socks_.begin());
	}
}

void ServerCore::closeConnections() {
	LOG_DEBUG("ServerCore: closing connections");
	while (!conns_.empty()) {
		// Cleans alway first element, no pointer update needed
		cleanConnection(conns_.begin()->first);
	}
}

int ServerCore::handleSocketEvent(int fd)
{
	std::map<int, Listener>::iterator	it;
	const ListeningSocket				*ls;
	std::ostringstream					msg;
	int									conn_fd;
	int									flags;
	int									saved_error;

	LOG_INFO("ServerCore: handling socket event -> port: " 
		<< socks_.at(fd).cfg_->getListen());
	it = socks_.find(fd);
	if (it == socks_.end())
	{
		LOG_WARNING("ServerCore: fd not found in socket map");
		return -1;
	}

	ls = it->second.sock_;
	conn_fd = ls->acceptClient();
	if (conn_fd < 0)
	{
		saved_error = errno;
		LOG_WARNING("ServerCore: accept failed, " << strerror(saved_error));

		// No siempre hay un cliente disponible cuando llegamos a accept().
		if (saved_error == EAGAIN || saved_error == EWOULDBLOCK
			|| saved_error == EINTR)
			return 0;

		return -1;
	}
	LOG_DEBUG("ServerCore: client accepted -> connection fd: " << conn_fd);

	flags = fcntl(conn_fd, F_GETFL, 0);
	if (flags < 0)
	{
		saved_error = errno;
		LOG_WARNING("ServerCore: fcntl failed, " << strerror(saved_error));
		close(conn_fd);
		return -1;
	}

	if (fcntl(conn_fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		saved_error = errno;
		LOG_WARNING("ServerCore: fcntl failed, " << strerror(saved_error));
		close(conn_fd);
		return -1;
	}
	LOG_DEBUG("ServerCore: connection fd set as non-blocking");

	if (createEvent(conn_fd, EPOLLIN) < 0)
	{
		saved_error = errno;
		LOG_WARNING("ServerCore: epoll ADD failed, " << strerror(saved_error));

		// Todavía no hay una Connection propietaria de este descriptor.
		// Lo cerramos para no dejarlo abierto en esta salida de error.
		close(conn_fd);
		return -1;
	}

	conns_[conn_fd] = new Connection(conn_fd, it->second.cfg_);
	LOG_INFO("ServerCore: new connection stablished -> fd: " << conn_fd);
	return conn_fd;
}

// Connections events handler
int ServerCore::handleConnectionEvent(struct epoll_event &event)
{
	std::map<int, Connection*>::iterator	it;
	Connection							*conn;
	std::ostringstream					msg;
	ssize_t								io_bytes;
	int									fd;
	int									saved_error;

	LOG_DEBUG("ServerCore: handling connection event -> fd: " << event.data.fd);
	fd = event.data.fd;
	it = conns_.find(fd);
	if (it == conns_.end())
	{
		LOG_WARNING("ServerCore: fd not found in connection map");
		return -1;
	}
	conn = it->second;

	if (event.events & EPOLLERR)
	{
		LOG_DEBUG("ServerCore: event type -> EPOLLERR");
		cleanConnection(fd);
		return -1;
	}

	if (event.events & EPOLLIN)
	{
		LOG_DEBUG("ServerCore: event type -> EPOLLIN");
		io_bytes = conn->readFromFd();
		LOG_DEBUG("ServerCore: read bytes -> " << io_bytes);

		if (io_bytes < 0)
		{
			saved_error = conn->getLastIoError();
			LOG_WARNING("ServerCore: recv failed, " << strerror(saved_error));
			cleanConnection(fd);
			return -1;
		}

		if (io_bytes == 0)
		{
			LOG_DEBUG("ServerCore: client disconnected");

			// Si ya hay una respuesta pendiente, conservamos el socket
			// para enviarla. El fin de lectura no prohíbe escribir.
			if (conn->wantsWrite())
			{
				if (modifyEvent(fd, EPOLLOUT) < 0)
				{
					saved_error = errno;
					LOG_WARNING("ServerCore: epoll MOD failed, " << strerror(saved_error));
					cleanConnection(fd);
					return -1;
				}
				return 0;
			}

			// Política actual: sin respuesta pendiente, cerramos.
			// No completamos una petición truncada usando EOF.
			cleanConnection(fd);
			return 0;
		}

		return static_cast<int>(io_bytes);
	}

	if (event.events & EPOLLHUP)
	{
		LOG_DEBUG("ServerCore: event type -> EPOLLHUP");
		cleanConnection(fd);
		return 0;
	}

	if (event.events & EPOLLOUT)
	{
		LOG_DEBUG("ServerCore: event type -> EPOLLOUT");
		io_bytes = conn->writeToFd();
		LOG_DEBUG("ServerCore: sent bytes -> " << io_bytes);

		if (io_bytes < 0)
		{
			saved_error = conn->getLastIoError();
			LOG_WARNING("ServerCore: send failed, " << strerror(saved_error));
			cleanConnection(fd);
			return -1;
		}

		if (io_bytes == 0 && conn->wantsWrite())
		{
			// No es una respuesta completada ni una confirmación de
			// desconexión. Cerramos por política para evitar repetir
			// indefinidamente una escritura sin progreso.
			LOG_WARNING("ServerCore: send made no progress");
			cleanConnection(fd);
			return -1;
		}

		if (!conn->wantsWrite())
		{
			LOG_DEBUG("ServerCore: response sending finished");
			if (modifyEvent(fd, EPOLLIN) < 0)
			{
				saved_error = errno;
				LOG_WARNING("ServerCore: epoll MOD failed, " << strerror(saved_error));
				cleanConnection(fd);
				return -1;
			}
			LOG_INFO("ServerCore: connection finished, closing...");
			cleanConnection(fd);
		}
		return static_cast<int>(io_bytes);
	}
	
	return 0;
}

int ServerCore::processConnections()
{
	std::map<int, Connection *>::const_iterator it;
	int fd;
	e_conn_state state_old;

	it = conns_.begin();
	for(; it != conns_.end(); ++it)
	{
		if (it->second->getProcessData())
		{
			while (it->second->getState() != FINALIZAED)
			{
				state_old = it->second->getState();
				it->second->advance();

				if (state_old == READING_HEADERS
					&& it->second->getState() == READING_HEADERS)
					break;
				if (state_old == READING_BODY
					&& it->second->getState() == READING_BODY)
					break;
			}

			if (it->second->getState() == FINALIZAED)
			{
				fd = it->second->getFd();
				if (modifyEvent(fd, EPOLLOUT) < 0)
				{
					cleanConnection(fd);
					return -1;
				}
			}

			it->second->setProcessData(false);
		}
		
	}
	
	return 0;
}

// Epoll error handling
int	ServerCore::logEpollError() const 
{
	int err = errno;
	LOG_WARNING("ServerCore: epoll_ctl failed, " << strerror(err));
	// Warnings: system fails or similar
	// if (err == ENOMEM) {
	// 	ft_log::global().warning("ServerCore: epoll_ctl failed,	no memory left on the device (ENOMEM)");
	// 	return 0;
	// }
	// if (err == ENOSPC) {
	// 	ft_log::global().warning("ServerCore: epoll_ctl failed,	max_users_watches limit reachede (ENOSPC)");
	// 	return 0;
	// }
	// // Real errors: bad epoll_ctl usage
	// if (err == EBADF) {
	// 	ft_log::global().error("ServerCore: epoll_ctl failed, not valid fd specified (EBADFD)");
	// 	return EBADF;
	// }
	// if (err == EEXIST) {
	// 	ft_log::global().error("ServerCore: epoll_ctl failed, fd is already on the epoll list (EEXIST)");
	// 	return EEXIST;
	// }
	// if (err == EINVAL) {
	// 	ft_log::global().error("ServerCore: epoll_ctl failed, invalid epoll operation tried (EINVAL)");
	// 	return EINVAL;
	// }
	// if (err == ENOENT) {
	// 	ft_log::global().error("ServerCore: epoll_ctl failed, tried to modify a non-registered fd (ENOENT)");
	// 	return ENOENT;
	// }
	// if (err == EPERM) {
	// 	ft_log::global().warning("ServerCore: epoll_ctl failed,	target fd doesnt support epoll (EPERM)");
	// 	return EPERM;
	// }
	// ft_log::global().critical("ServerCore: epoll_ctl failed, UNKNOWN ERROR");
	return -1;
}

// Public Methods -------------------------------------------------------------
int ServerCore::setup() {
	LOG_DEBUG("ServerCore: setting up...");
	initSockets();
	return 0;
}

void ServerCore::timeConnections()
{
	std::map<int, Connection*>::const_iterator	it;
	int		fd;
	int		timeout;							

	it = conns_.begin();
	while (it != conns_.end())
	{
		if (it->second->getInactiveSeconds() >= TIMEOUT_CONNECTION
			&& (it->second->getState() == READING_HEADERS
				|| it->second->getState() == READING_BODY))
		{

			fd = it->second->getFd();
			timeout = it->second->getInactiveSeconds();
			LOG_INFO("ServerCore: connection timeout -> "
				<< "fd: " << fd
				<< ", inactive time: " << timeout << "s"
			);

			cleanConnection(fd);
			++it;
		}
		else
			++it;
	}
}

int ServerCore::run() 
{
	LOG_INFO("ServerCore: running...");
	struct epoll_event	events[EVENTS_MAX];
	int					event_count;
	int					saved_error;
	std::ostringstream	msg;

	//fd_ Serverfd_ loop
	while (!socks_.empty()) 
	{
		event_count = epoll_wait(epoll_fd_, events,
				EVENTS_MAX, TIMEOUT_EPOLL);

		if (event_count < 0)
		{
			saved_error = errno;

			// Una señal puede interrumpir epoll_wait() sin que epoll esté roto.
			if (saved_error == EINTR)
			{
				timeConnections();
				continue;
			}

			LOG_CRITICAL("ServerCore: epoll_wait failed, " << strerror(saved_error));
			return -1;
		}
		
		// Handle events
		for (int i = 0; i < event_count; ++i) 
		{

			int cur_fd = events[i].data.fd;
			// If the fd is in connection map
			if (conns_.count(cur_fd)) 
				handleConnectionEvent(events[i]);
			else 
				handleSocketEvent(cur_fd);
			
		}
		processConnections();
		timeConnections();

	}
	LOG_CRITICAL("No listening ports left open. Closing server...");
	return 0;
}
