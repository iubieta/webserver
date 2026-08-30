// Connection.cpp
// ----------------------------------------------------------------------------

#include "../inc/Connection.hpp"
#include "../inc/log_global.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>

// Constructors ---------------------------------------------------------------
Connection::Connection(int fd) : fd_(fd), closed_(false), write_offset_(0) {
	std::ostringstream msg;
	msg << "Connection: obj created (fd: " << fd_ << ")"; 
	log::global().debug(msg.str(), __FILE__, __LINE__);
}

Connection::~Connection() {
	if (fd_ != -1) {
		close(fd_);
		std::ostringstream msg;
		msg << "Connection: closing (fd: " << fd_ << ")"; 
		log::global().debug(msg.str(), __FILE__, __LINE__);
	}
}

// Public methods -------------------------------------------------------------
ssize_t Connection::readFromFd() {
	if (fd_ == -1) {
		return -1;
	}
	char	temp[BUFFLEN];
	ssize_t read_bytes = recv(fd_, temp, BUFFLEN, 0);
	if (read_bytes > 0) {
		read_buff_.append(temp, read_bytes);
		std::ostringstream msg;
		msg << "Connection: RECEIVED -> " << temp; 
		log::global().debug(msg.str(), __FILE__, __LINE__);
	}
	return read_bytes;
}

ssize_t Connection::writeToFd() {
	if (write_offset_ >= write_buff_.size())
		return 0;
	const char *temp = write_buff_.c_str() + write_offset_;
	ssize_t sent_bytes = send(fd_, temp, BUFFLEN, 0);
	if (sent_bytes > 0)
		write_offset_ += sent_bytes;
	return sent_bytes;
}

void Connection::appendToWriteBuff(const std::string &data) {
	write_buff_.append(data);
	std::ostringstream msg;
	msg << "Connection: WRITE BUFFER -> " <<  write_buff_; 
	log::global().debug(msg.str(), __FILE__, __LINE__);
}

void Connection::disconnect() {
	if (fd_ != -1) {
		std::ostringstream msg;
		msg << "Connection: closing fd (fd: " << fd_ << ")"; 
		log::global().debug(msg.str(), __FILE__, __LINE__);
		close(fd_);
		fd_ = -1;
	}
	closed_ = true;
}

bool Connection::wantsWrite() const {
	return write_offset_ < write_buff_.size();
}

bool Connection::isClosed() const {
	return closed_;
}
