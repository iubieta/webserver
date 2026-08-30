// Connection.hpp
// ----------------------------------------------------------------------------

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <cstddef>
#include <string>
#include <sys/types.h>

#define BUFFLEN 32768 // 32 KB

class Connection {
private:
	int				fd_;
	bool			closed_;
	std::string		read_buff_;
	std::string		write_buff_;
	size_t			write_offset_;

	Connection(const Connection &other);
	Connection& operator=(const Connection &other);

public:
	Connection(int fd);
	~Connection();

	ssize_t		readFromFd();
	ssize_t		writeToFd();
	void		appendToWriteBuff(const std::string &data); // Used in response builder
	void		disconnect();
	bool		wantsWrite() const;	// There is data on the write buffer
	bool		isClosed() const;	// Client closed the connection
};

#endif
