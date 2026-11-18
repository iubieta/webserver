// Connection.cpp
// ----------------------------------------------------------------------------

#include "../inc/Connection.hpp"
#include "../inc/log_global.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <cerrno>

// Constructors ---------------------------------------------------------------
Connection::Connection(int fd, const ServerConfig* config) : fd_(fd), config_(config),
	closed_(false), last_activity_(time(NULL)), state_(READING_HEADERS), 
	parser_(), request_(), processor_(config), decision_(), response_(),
	last_io_error_(0)
{
	std::ostringstream msg;
	msg << "Connection: obj created (fd: " << fd_ << ")"; 
	ft_log::global().debug(msg.str(), __FILE__, __LINE__);
}

Connection::~Connection() {
	if (fd_ != -1) {
		close(fd_);
		std::ostringstream msg;
		msg << "Connection: closing (fd: " << fd_ << ")"; 
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
	}
}

// Public methods -------------------------------------------------------------

// Status control
int Connection::getFd() const {
	return fd_;
}

bool Connection::isClosed() const {
	return closed_;
}

bool Connection::wantsWrite() const {
	return !write_buff_.empty();
}

time_t Connection::getLastActivity() const {
	return last_activity_;
}

const Buffer &Connection::getReadBuffer() const
{
	return read_buff_;
}

void Connection::updateLastActivity()
{
	std::stringstream ss;
	last_activity_ = std::time(NULL);

	ss << "fd = "<< fd_  << " time reset " << last_activity_;
	ft_log::global().debug(ss.str(), __FILE__, __LINE__);
}

int Connection::getLastIoError() const
{
	return last_io_error_;
}

double Connection::getInactiveSeconds() const
{
	std::time_t	now;
	double	eleapsed;

	now = std::time(NULL);
	eleapsed = std::difftime(now, last_activity_);
	return eleapsed;
}

void Connection::disconnect() {
	if (fd_ != -1) {
		std::ostringstream msg;
		msg << "Connection: closing fd (fd: " << fd_ << ")"; 
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
		close(fd_);
		fd_ = -1;
	}
	closed_ = true;
}

// System I/0
ssize_t Connection::readFromFd() 
{
	char	temp[BUFFLEN];
	ssize_t read_bytes;

	last_io_error_ = 0;

	if (fd_ == -1) 
	{
		last_io_error_ = EBADF;
		return -1;
	}

	
	read_bytes = recv(fd_, temp, BUFFLEN, 0);
	
	if (read_bytes < 0)
	{
		last_io_error_ = errno;
		return read_bytes;
	}
	// Normal receive
	if (read_bytes > 0) 
	{
		read_buff_.append(temp, read_bytes);
		updateLastActivity();
	}

	return read_bytes;
}

ssize_t Connection::writeToFd()
{
	const char	*data;
	size_t		write_len;
	ssize_t		sent_bytes;

	last_io_error_ = 0;

	if (fd_ == -1)
	{
		last_io_error_ = EBADF;
		return -1;
	}

	if (write_buff_.empty())
		return 0;

	data = write_buff_.data();
	write_len = write_buff_.size();
	if (write_len > BUFFLEN)
		write_len = BUFFLEN;

	// En Linux, MSG_NOSIGNAL evita que una escritura en un socket roto
	// termine el proceso por SIGPIPE. send() seguirá devolviendo el error.
	sent_bytes = send(fd_, data, write_len, MSG_NOSIGNAL);

	if (sent_bytes < 0)
	{
		last_io_error_ = errno;
		return sent_bytes;
	}

	if (sent_bytes > 0)
		write_buff_.consume(sent_bytes);

	return sent_bytes;
}

// Read buffer
const char *Connection::readData() {
	return read_buff_.data();
}

size_t	Connection::readSize() {
	return read_buff_.size();
}

void Connection::consume(size_t n) {
	read_buff_.consume(n);
}

// Write buffer
void Connection::appendToWrite(const std::string &data) {
	write_buff_.append(data);
	
	std::ostringstream msg;
	msg << "Connection: WRITE BUFFER -> ";
	msg.write(write_buff_.data(), write_buff_.size());
	ft_log::global().debug(msg.str(), __FILE__, __LINE__);
}

void Connection::appendToWrite(const char *data, size_t n) {
	write_buff_.append(data, n);
	
	std::ostringstream msg;
	msg << "Connection: WRITE BUFFER -> "; 
	msg.write(write_buff_.data(), write_buff_.size());
	ft_log::global().debug(msg.str(), __FILE__, __LINE__);
}

e_conn_state Connection::getState() const {
	return state_;
}

void	Connection::advance() 
{
	e_parser_state	parser_state;
	size_t client_max_body = 0;
	
	switch (state_) 
	{
		case READING_HEADERS:
		{
			parser_state = parser_.stateHandler(read_buff_, request_);
			if (parser_state == BODY 
				|| parser_state == COMPLETE 
				|| parser_state == ERROR)
				state_ = PROCESSING;
			break;
		}
		case PROCESSING:
		{
	 		decision_ = processor_.procRequest(request_);

			if (decision_.getStatus() == OK && parser_.getState() == BODY)
			{
				if (decision_.getLocationConfig() != NULL)
					client_max_body = decision_.getLocationConfig()->getClientMax();
				if (client_max_body > 0)
					parser_.setMaxbodySize(client_max_body);
				state_ = READING_BODY;
			}
			else
				state_ = RESPONDING;
			break;
		}
		case READING_BODY:
		{
			
			parser_state = parser_.bodyParser(read_buff_, request_);
			if (parser_state == COMPLETE) 
			{
				decision_.setBody(request_.getBody());
				state_ = RESPONDING;
			}
			else if (parser_state == ERROR) {
				state_ = PROCESSING;
			}
			break;
		}
		case RESPONDING:
		{
			response_ = builder_.execute(decision_);
			write_buff_.append(response_.build());
			state_ = FINALIZAED;
			break;
		}
		case FINALIZAED: 
		{
			break;
		}
		
	}
}
