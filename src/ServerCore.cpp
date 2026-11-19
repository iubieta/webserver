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
#include "../inc/log_global.hpp"
#include "../inc/Logger.hpp"

// Constructors ---------------------------------------------------------------
ServerCore::ServerCore(std::vector<ServerConfig> configs) : configs_(configs) {
	epoll_fd_ = epoll_create(1);
	if (epoll_fd_ < 0) {
		ft_log::global().critical("ServerCore: Epoll creation failed", __FILE__, __LINE__);
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
	ft_log::global().debug("ServerCore: initializing server sockets", __FILE__, __LINE__);

	std::vector<ServerConfig>::iterator it;
	std::vector<ServerConfig>::iterator ite = configs_.end();
	// For each config init a socket
	for (it = configs_.begin(); it != ite; ++it) {

		// Create new socket
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
		// If epoll adding fails delete the socket
		if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event.data.fd, &event) < 0) {
			logEpollError();
			delete ls;
			continue;
		}
		
		// Add the socket to listening socket map
		socks_.insert(
			std::pair<int, Listener>(ls->getFd(), Listener(ls, &(*it)))
		);
		std::ostringstream msg;
		msg << "ServerCore: socket correctly initiated (fd: " << ls->getFd() << ")";
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
	}
}

// Connection cleaning 
void ServerCore::cleanConnection(int fd)
{
	std::map<int, Connection*>::iterator	it;
	std::ostringstream					msg;
	int									saved_error;

	it = conns_.find(fd);
	if (it == conns_.end())
	{
		// operator[] insertaría una entrada si el fd no existe.
		// find() permite comprobarlo sin modificar el mapa.
		msg << "ServerCore: cleanup requested for unknown fd=" << fd;
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
		return;
	}

	if (deleteEvent(fd) < 0)
	{
		saved_error = errno;
		msg << "ServerCore: epoll DEL failed fd=" << fd
			<< " errno=" << saved_error
			<< " (" << std::strerror(saved_error) << ")";
		ft_log::global().error(msg.str(), __FILE__, __LINE__);
		msg.str("");
		msg.clear();
	}

	// Aunque falle la retirada de epoll, intentamos liberar el cliente.
	it->second->disconnect();
	delete it->second;
	conns_.erase(it);

	// Confirmamos la limpieza después de realizarla.
	msg << "ServerCore: connection cleaned fd=" << fd;
	ft_log::global().debug(msg.str(), __FILE__, __LINE__);
}

void ServerCore::closeSockets() {
	ft_log::global().debug("ServerCore: closing server sockets", __FILE__, __LINE__);
	while (!socks_.empty()) {
		delete socks_.begin()->second.sock_;
		socks_.erase(socks_.begin());
	}
}

void ServerCore::closeConnections() {
	ft_log::global().debug("ServerCore: closing server connections", __FILE__, __LINE__);
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

	it = socks_.find(fd);
	if (it == socks_.end())
	{
		msg << "ServerCore: event for unknown descriptor fd=" << fd;
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
		return -1;
	}

	ls = it->second.sock_;
	conn_fd = ls->acceptClient();
	if (conn_fd < 0)
	{
		saved_error = errno;

		// No siempre hay un cliente disponible cuando llegamos a accept().
		if (saved_error == EAGAIN || saved_error == EWOULDBLOCK
			|| saved_error == EINTR)
			return 0;

		msg << "ServerCore: accept failed listener_fd=" << fd
			<< " errno=" << saved_error
			<< " (" << std::strerror(saved_error) << ")";
		ft_log::global().warning(msg.str(), __FILE__, __LINE__);
		return -1;
	}

	flags = fcntl(conn_fd, F_GETFL, 0);
	if (flags < 0)
	{
		saved_error = errno;
		msg << "ServerCore: fcntl GETFL failed fd=" << conn_fd
			<< " errno=" << saved_error
			<< " (" << std::strerror(saved_error) << ")";
		ft_log::global().error(msg.str(), __FILE__, __LINE__);
		close(conn_fd);
		return -1;
	}

	if (fcntl(conn_fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		saved_error = errno;
		msg << "ServerCore: fcntl SETFL failed fd=" << conn_fd
			<< " errno=" << saved_error
			<< " (" << std::strerror(saved_error) << ")";
		ft_log::global().error(msg.str(), __FILE__, __LINE__);
		close(conn_fd);
		return -1;
	}

	if (createEvent(conn_fd, EPOLLIN) < 0)
	{
		saved_error = errno;
		msg << "ServerCore: epoll ADD failed fd=" << conn_fd
			<< " errno=" << saved_error
			<< " (" << std::strerror(saved_error) << ")";
		ft_log::global().error(msg.str(), __FILE__, __LINE__);

		// Todavía no hay una Connection propietaria de este descriptor.
		// Lo cerramos para no dejarlo abierto en esta salida de error.
		close(conn_fd);
		return -1;
	}

	conns_[conn_fd] = new Connection(conn_fd, it->second.cfg_);

	msg << "ServerCore: connection accepted listener_fd=" << fd
		<< " client_fd=" << conn_fd;
	ft_log::global().info(msg.str(), __FILE__, __LINE__);
	return conn_fd;
}

// Connections events handler
int ServerCore::handleConnectionEvent(struct epoll_event &event)
{
	std::map<int, Connection*>::iterator	it;
	Connection							*conn;
	std::ostringstream					msg;
	ssize_t								io_bytes;
	e_conn_state						state_old;
	int									fd;
	int									saved_error;

	fd = event.data.fd;
	it = conns_.find(fd);
	if (it == conns_.end())
		return -1;
	conn = it->second;

	if (event.events & EPOLLERR)
	{
		msg << "ServerCore: socket error event fd=" << fd;
		ft_log::global().warning(msg.str(), __FILE__, __LINE__);
		cleanConnection(fd);
		return -1;
	}

	if (event.events & EPOLLIN)
	{
		io_bytes = conn->readFromFd();

		if (io_bytes < 0)
		{
			saved_error = conn->getLastIoError();

			msg << "ServerCore: recv failed fd=" << fd
				<< " errno=" << saved_error
				<< " (" << std::strerror(saved_error) << ")";
			ft_log::global().warning(msg.str(), __FILE__, __LINE__);
			cleanConnection(fd);
			return -1;
		}

		if (io_bytes == 0)
		{
			msg << "ServerCore: peer finished sending fd=" << fd
				<< " state=" << conn->getState();
			ft_log::global().debug(msg.str(), __FILE__, __LINE__);

			// Si ya hay una respuesta pendiente, conservamos el socket
			// para enviarla. El fin de lectura no prohíbe escribir.
			if (conn->wantsWrite())
			{
				if (modifyEvent(fd, EPOLLOUT) < 0)
				{
					saved_error = errno;
					msg.str("");
					msg.clear();
					msg << "ServerCore: epoll MOD failed fd=" << fd
						<< " errno=" << saved_error
						<< " (" << std::strerror(saved_error) << ")";
					ft_log::global().error(msg.str(), __FILE__, __LINE__);
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

		msg << "ServerCore: received fd=" << fd
			<< " bytes=" << io_bytes;
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);

		while (conn->getState() != FINALIZAED)
		{
			state_old = conn->getState();
			conn->advance();

			if (state_old == READING_HEADERS
				&& conn->getState() == READING_HEADERS)
				break;
			if (state_old == READING_BODY
				&& conn->getState() == READING_BODY)
				break;
		}

		if (conn->getState() == FINALIZAED)
		{
			if (modifyEvent(fd, EPOLLOUT) < 0)
			{
				saved_error = errno;
				msg.str("");
				msg.clear();
				msg << "ServerCore: epoll MOD failed fd=" << fd
					<< " errno=" << saved_error
					<< " (" << std::strerror(saved_error) << ")";
				ft_log::global().error(msg.str(), __FILE__, __LINE__);

				// No dejamos una respuesta esperando sin una vigilancia
				// de escritura que sepamos que se instaló correctamente.
				cleanConnection(fd);
				return -1;
			}
		}
		return static_cast<int>(io_bytes);
	}

	if (event.events & EPOLLHUP)
	{
		msg << "ServerCore: socket hangup fd=" << fd;
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
		cleanConnection(fd);
		return 0;
	}

	if (event.events & EPOLLOUT)
	{
		io_bytes = conn->writeToFd();

		if (io_bytes < 0)
		{
			saved_error = conn->getLastIoError();

			msg << "ServerCore: send failed fd=" << fd
				<< " errno=" << saved_error
				<< " (" << std::strerror(saved_error) << ")";
			ft_log::global().warning(msg.str(), __FILE__, __LINE__);
			cleanConnection(fd);
			return -1;
		}

		if (io_bytes == 0 && conn->wantsWrite())
		{
			// No es una respuesta completada ni una confirmación de
			// desconexión. Cerramos por política para evitar repetir
			// indefinidamente una escritura sin progreso.
			msg << "ServerCore: send made no progress fd=" << fd;
			ft_log::global().warning(msg.str(), __FILE__, __LINE__);
			cleanConnection(fd);
			return -1;
		}

		msg << "ServerCore: sent fd=" << fd
			<< " bytes=" << io_bytes;
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);

		if (!conn->wantsWrite())
		{
			msg.str("");
			msg.clear();
			msg << "ServerCore: response sent, closing fd=" << fd;
			ft_log::global().debug(msg.str(), __FILE__, __LINE__);
			cleanConnection(fd);
		}
		return static_cast<int>(io_bytes);
	}

	
	return 0;
}

// Epoll error handling
int	ServerCore::logEpollError() const 
{
	int err = errno;
	// Warnings: system fails or similar
	if (err == ENOMEM) {
		ft_log::global().warning("ServerCore: epoll_ctl failed,	no memory left on the device (ENOMEM)");
		return 0;
	}
	if (err == ENOSPC) {
		ft_log::global().warning("ServerCore: epoll_ctl failed,	max_users_watches limit reachede (ENOSPC)");
		return 0;
	}
	// Real errors: bad epoll_ctl usage
	if (err == EBADF) {
		ft_log::global().error("ServerCore: epoll_ctl failed, not valid fd specified (EBADFD)");
		return EBADF;
	}
	if (err == EEXIST) {
		ft_log::global().error("ServerCore: epoll_ctl failed, fd is already on the epoll list (EEXIST)");
		return EEXIST;
	}
	if (err == EINVAL) {
		ft_log::global().error("ServerCore: epoll_ctl failed, invalid epoll operation tried (EINVAL)");
		return EINVAL;
	}
	if (err == ENOENT) {
		ft_log::global().error("ServerCore: epoll_ctl failed, tried to modify a non-registered fd (ENOENT)");
		return ENOENT;
	}
	if (err == EPERM) {
		ft_log::global().warning("ServerCore: epoll_ctl failed,	target fd doesnt support epoll (EPERM)");
		return EPERM;
	}
	ft_log::global().critical("ServerCore: epoll_ctl failed, UNKNOWN ERROR");
	return -1;
}

// Public Methods -------------------------------------------------------------
int ServerCore::setup() {
	ft_log::global().debug("ServerCore: setup", __FILE__, __LINE__);
	initSockets();
	return 0;
}

void ServerCore::timeConnections()
{
	std::map<int, Connection*>::const_iterator	it;
	int											fd;

	std::stringstream ss;


	it = conns_.begin();
	while (it != conns_.end())
	{
		if (it->second->getInactiveSeconds() >= TIMEOUT_CONNECTION
			&& (it->second->getState() == READING_HEADERS
				|| it->second->getState() == READING_BODY))
		{
			fd = it->second->getFd();
			ss.str("");
			ss.clear();

			ss << "ServerCore: receive timeout fd=" << fd
				<< " inactive_seconds=" << it->second->getInactiveSeconds();

			ft_log::global().info(ss.str(), __FILE__, __LINE__);
			++it;
			cleanConnection(fd);
		}
		else
			++it;
	}
}

int ServerCore::run() 
{
	ft_log::global().info("server initialized");
	struct epoll_event	events[EVENTS_MAX];
	int					event_count;
	int					saved_error;
	std::ostringstream	msg;

	// Server loop
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

			msg << "ServerCore: epoll_wait failed errno=" << saved_error
				<< " (" << std::strerror(saved_error) << ")";
			ft_log::global().critical(msg.str(), __FILE__, __LINE__);
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

		timeConnections();

	}
	ft_log::global().critical("No listening ports open. Closing server...");
	return 0;
}
