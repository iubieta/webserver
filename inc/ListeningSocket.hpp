// ListeningSocket.hpp
// ----------------------------------------------------------------------------

#ifndef LISTENINGSOCKET_HPP
#define LISTENINGSOCKET_HPP

#include <string>

#define BACKLOG 1000

class ListeningSocket {
private:
	unsigned int	host_;
	unsigned int	port_;
	int				fd_;

	ListeningSocket(const ListeningSocket &other);
	ListeningSocket& operator=(const ListeningSocket &other);

public:
	ListeningSocket(unsigned int host, unsigned int port);
	~ListeningSocket();

	unsigned int		getHost() const;
	unsigned int		getPort() const;
	int					getFd() const;
	
	bool				setup();
	bool				isReady() const;
	int					acceptClient() const;
};

#endif // !LISTENINGSOCKET_HPP
