// ListeningSocket.hpp
// ----------------------------------------------------------------------------

#ifndef LISTENINGSOCKET_HPP
#define LISTENINGSOCKET_HPP

#include <string>

#define BACKLOG 1000

class ListeningSocket {
private:
	std::string		host_;
	int				port_;
	int				fd_;

	ListeningSocket(const ListeningSocket &other);
	ListeningSocket& operator=(const ListeningSocket &other);

public:
	ListeningSocket(const std::string &host, int port);
	~ListeningSocket();

	const std::string	&getHost() const;
	int					getPort() const;
	int					getFd() const;
	
	bool				setup();
	bool				isReady() const;
	int					acceptClient() const;
};

#endif // !LISTENINGSOCKET_HPP
