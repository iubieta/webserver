// Connection.cpp
// ----------------------------------------------------------------------------

#include "../inc/Connection.hpp"
#include "../inc/log_global.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>

// Constructors ---------------------------------------------------------------
Connection::Connection(int fd) : fd_(fd), closed_(false) {
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
ssize_t Connection::readFromFd() {
	// Conncetion already closed
	if (fd_ == -1) {
		std::ostringstream msg;
		msg << "Connection: fd already closed -> fd = " << fd_; 
		ft_log::global().error(msg.str(), __FILE__, __LINE__);

		// return IO_ERROR
		return -1;
	}

	char	temp[BUFFLEN];
	ssize_t read_bytes = recv(fd_, temp, BUFFLEN, 0);
	
	// Normal receive
	if (read_bytes > 0) {
		read_buff_.append(temp, read_bytes);

		std::ostringstream msg;
		msg << "Connection: RECEIVED -> ";
		msg.write(temp, read_bytes);
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
	}

	// Client dissconnection
	if (read_bytes == 0) {
		std::ostringstream msg;
		msg << "Connection: client disconnected -> fd = " << fd_; 
		ft_log::global().error(msg.str(), __FILE__, __LINE__);

		// return IO_CLOSING;
		return 0;
	}
	return read_bytes;
}

ssize_t Connection::writeToFd() {
	const char *temp = write_buff_.data();
	ssize_t write_len = BUFFLEN;

	// Adjust sending buffer length
	if (write_buff_.size() < BUFFLEN) {
		write_len = write_buff_.size(); 
	}

	// Sending
	ssize_t sent_bytes = send(fd_, temp, write_len, 0);

	// Consume sent bytes from the buffer
	if (sent_bytes > 0) {
		std::string sent(temp, sent_bytes);
		
		write_buff_.consume(sent_bytes);

		std::ostringstream msg;
		msg << "Connection: SENT -> " <<  sent; 
		ft_log::global().debug(msg.str(), __FILE__, __LINE__);
	}

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


