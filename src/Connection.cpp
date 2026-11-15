// Connection.cpp
// ----------------------------------------------------------------------------

#include "../inc/Connection.hpp"
#include "../inc/log_global.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>

// Constructors ---------------------------------------------------------------
Connection::Connection(int fd, const ServerConfig* config) : fd_(fd), config_(config),
	closed_(false), last_activity_(time(NULL)), state_(READING_HEADERS), 
	parser_(), request_(), processor_(config), decision_(), response_()
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

	// Conncetion already closed
	if (fd_ == -1) {
		std::ostringstream msg;
		msg << "Connection: fd already closed -> fd = " << fd_; 
		ft_log::global().error(msg.str(), __FILE__, __LINE__);

		// return IO_ERROR
		return -1;
	}

	
	read_bytes = recv(fd_, temp, BUFFLEN, 0);
	
	// Normal receive
	if (read_bytes > 0) 
	{
		read_buff_.append(temp, read_bytes);
		
		// Update last activity time
		last_activity_ = time(NULL);
	}

	// Client dissconnection
	if (read_bytes == 0) {
		std::ostringstream msg;
		msg << "Connection: client disconnected "
			<< " (Last Activity: " << last_activity_ << ")";
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
	
	// Update last activity time
	last_activity_ = time(NULL);

	// Consume sent bytes from the buffer
	if (sent_bytes > 0) {
		std::string sent(temp, sent_bytes);
		
		write_buff_.consume(sent_bytes);

		// std::ostringstream msg;
		// msg << "Connection: SENT -> " <<  sent; 
		// msg << " (Last Activity: " << last_activity_ << ")";
		// ft_log::global().debug(msg.str(), __FILE__, __LINE__);
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
