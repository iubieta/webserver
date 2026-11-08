// Connection.hpp
// ----------------------------------------------------------------------------

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <cstddef>
#include <string>
#include <sys/types.h>

#include "Buffer.hpp"
#include "ServerConfig.hpp"
#include "HttpParser.hpp"
#include "Decision.hpp"
//#include "../inc/Request.hpp"

#define BUFFLEN 32768 // 32 KB

enum e_conn_state {
	READING_HEADERS,
	PROCESSING,
	READING_BODY,
	EXECUTING,
	RESPONDING,
	CLOSING,
	KEEP_ALIVE,
};

class Connection {
private:
	// ServerCore related
	int					fd_;
	const ServerConfig*		config_;

	bool				closed_;
	Buffer				read_buff_;
	Buffer				write_buff_;
	time_t				last_activity_;
	e_conn_state		state_;
 
	// Request related
	HttpParser  parser_;
	Request     request_;
	Decision	decision_;

	Connection(const Connection &other);
	Connection& operator=(const Connection &other);

public:
	// TODO: add ServerConfig to the constructor
	Connection(int fd, ServerConfig* config);
	~Connection();

	// Status control
	int			getFd() const;
	bool		isClosed() const;	// Client closed the connection
	bool		wantsWrite() const;	// There is data on the write buffer
	time_t		getLastActivity() const;
	void		disconnect();

	// State control
	void		setState(e_conn_state state);
	void		advance();
	
	// System I/O
	ssize_t		readFromFd();
	ssize_t		writeToFd();
	
	// Read buffer
	const char	*readData(); // Used in response builder
	size_t		readSize();
	void		consume(size_t n); // Discard n bytes from read buffer
	
	// Write buffer
	void		appendToWrite(const std::string &data); // Used in response builder
	void		appendToWrite(const char *data, size_t n); // Used in response builder
};

#endif
