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
	size_t			read_offset_;
	std::string		write_buff_;
	size_t			write_offset_;

	Connection(const Connection &other);
	Connection& operator=(const Connection &other);

public:
	Connection(int fd);
	~Connection();

	int			getFd() const;
	bool		isClosed() const;	// Client closed the connection
	size_t		getReadOffset() const;
	size_t		getWriteOffset() const;
	void		disconnect();
	
	ssize_t		readFromFd();
	ssize_t		writeToFd();
	
	
	const std::string&	readBuff(); // Used in response builder
	void		consume(size_t n); // Discard n bytes from read buffer
	void		clearReadBuff(); // Used in response builder
	
	void		appendToWriteBuff(const std::string &data); // Used in response builder
	bool		wantsWrite() const;	// There is data on the write buffer
	void		clearWriteBuff(); // Used in response builder
};

#endif
