// Connection.hpp
// ----------------------------------------------------------------------------

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <cstddef>
#include <string>
#include <sys/types.h>

#include "../inc/Buffer.hpp"

#define BUFFLEN 32768 // 32 KB

// TODO: define status macros
// IO_ERROR = -1
// IO_CLOSING = 0
// IO_OK > 0

class Connection {
private:
	int			fd_;
	bool		closed_;
	Buffer		read_buff_;
	Buffer		write_buff_;
	time_t		last_activity_;

	Connection(const Connection &other);
	Connection& operator=(const Connection &other);

public:
	Connection(int fd);
	~Connection();

	// Status control
	int			getFd() const;
	bool		isClosed() const;	// Client closed the connection
	bool		wantsWrite() const;	// There is data on the write buffer
	time_t		getLastActivity() const;
	void		disconnect();
	
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
