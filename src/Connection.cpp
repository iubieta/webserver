// Connection.cpp
// ----------------------------------------------------------------------------

#include "../inc/Connection.hpp"
#include "../inc/log_macros.hpp"
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <cerrno>

// Constructors ---------------------------------------------------------------
Connection::Connection(int fd, const ServerConfig* config) : fd_(fd), config_(config),
	closed_(false), last_activity_(time(NULL)), state_(READING_HEADERS), 
	parser_(), request_(), processor_(config), decision_(), response_(),
	last_io_error_(0), process_data_(false)
{
	std::ostringstream msg;
	msg << "Connection: obj created (fd: " << fd_ << ")"; 
	ft_log::global().debug(msg.str(), __FILE__, __LINE__);
}

Connection::~Connection() {
	LOG_DEBUG("Connection: destructor called");
	if (fd_ != -1) {
		close(fd_);
		LOG_DEBUG("Connection: closing fd: " << fd_);
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
	LOG_DEBUG("Connection: updated last activity time: " << last_activity_);
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
		LOG_DEBUG("Connection: closing fd: " << fd_);
		close(fd_);
		fd_ = -1;
	}
	closed_ = true;
}


bool Connection::getProcessData() const
{
	return process_data_;
}

void Connection::setProcessData(bool processData)
{
	process_data_ = processData;
}

// System I/0
ssize_t Connection::readFromFd() 
{
	char	temp[BUFFLEN];
	ssize_t read_bytes;

	last_io_error_ = 0;

	if (fd_ == -1) 
	{
		LOG_WARNING("Connection: imposible to read, no open fd");
		last_io_error_ = EBADF;
		return -1;
	}

	read_bytes = recv(fd_, temp, BUFFLEN, 0);
	LOG_DEBUG("Connection " << read_bytes << " bytes read");
	if (read_bytes < 0)
	{
		LOG_WARNING("Connection: receiving failed");
	}
	// Normal receive
	if (read_bytes > 0) 
	{
		read_buff_.append(temp, read_bytes);
		LOG_DEBUG("Connection: bytes appended to read buffer");
		process_data_ = true;
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
		LOG_WARNING("Connection: imposible to write, no open fd");
		return -1;
	}

	if (write_buff_.empty()) {
		LOG_WARNING("Connection: imposible to write, buffer empty");
		return 0;
	}

	data = write_buff_.data();
	write_len = write_buff_.size();
	if (write_len > BUFFLEN)
		write_len = BUFFLEN;

	// En Linux, MSG_NOSIGNAL evita que una escritura en un socket roto
	// termine el proceso por SIGPIPE. send() seguirá devolviendo el error.
	sent_bytes = send(fd_, data, write_len, MSG_NOSIGNAL);
	LOG_DEBUG("Connection: " << sent_bytes << " bytes sent");

	if (sent_bytes < 0)
	{
		LOG_WARNING("Connection: sending failed");
		return sent_bytes;
	}

	if (sent_bytes > 0) {
		write_buff_.consume(sent_bytes);
		LOG_DEBUG("Connection: bytes consumed from write buffer");
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
}

void Connection::appendToWrite(const char *data, size_t n) {
	write_buff_.append(data, n);
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
			{
				LOG_DEBUG("Connection: request received -> fd: " << fd_);
				LOG_INFO("--- REQUEST: "
					<< request_.getMethod() << " "
					<< request_.getPath() << " "
					<< request_.getQuery() << " "
					<< request_.getVersion() << " "
					<< "---"
				);
				state_ = PROCESSING;
				LOG_DEBUG("Connection: switched to PROCESSING state");
			}
			break;
		}
		case PROCESSING:
		{
	 		decision_ = processor_.procRequest(request_);
			LOG_DEBUG("Connection: decision took");

			if (decision_.getStatus() == OK && parser_.getState() == BODY)
			{
				if (decision_.getLocationConfig() != NULL)
					client_max_body = decision_.getLocationConfig()->getClientMax();
				if (client_max_body > 0)
					parser_.setMaxbodySize(client_max_body);
				state_ = READING_BODY;
				LOG_DEBUG("Connection: switched to READING_BODY state");
			}
			else
			{
				state_ = RESPONDING;
				LOG_DEBUG("Connection: switched to RESPONDING state");
			}
			break;
		}
		case READING_BODY:
		{
			parser_state = parser_.bodyParser(read_buff_, request_);
			if (parser_state == COMPLETE) 
			{
				decision_.setBody(request_.getBody());
				state_ = RESPONDING;
				LOG_DEBUG("Connection: switched to RESPONDING state");
			}
			else if (parser_state == ERROR) {
				state_ = PROCESSING;
				LOG_DEBUG("Connection: switched to PROCESSING state");
			}
			break;
		}
		case RESPONDING:
		{
			response_ = builder_.execute(decision_);
			LOG_DEBUG("Connection: response generated");
			LOG_INFO("--- RESPONSE: "
				<< response_.getVersion() << " "
				<< response_.getStatusCode() << " "
				<< response_.getReasonPhrase() << " "
				<< "---"
			);
			write_buff_.append(response_.build());
			// TODO: decide to remove finalized state
			state_ = FINALIZAED;
			LOG_DEBUG("Connection: switch to process ending");
			break;
		}
		case FINALIZAED: 
		{
			//LOG_DEBUG("Connection: PROCESS FINISHED");
			break;
		}
		
	}
}
