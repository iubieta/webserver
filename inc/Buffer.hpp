// Buffer.hpp
// ----------------------------------------------------------------------------

#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <string>

class Buffer {
private:
	std::string data_;
	size_t		offset_;

public:
	Buffer();
	Buffer(const Buffer &other);
	Buffer& operator=(const Buffer&other);
	~Buffer();

	const char	*data() const;
	size_t		size() const;
	bool		empty() const;
	void		append(const std::string &str);
	void		append(const char* str, size_t n);
	void		consume(size_t n);

	size_t		find(const std::string &delimiter) const;
	std::string	substr(size_t pos, size_t n) const;
};

#endif // !BUFFER_HPP
