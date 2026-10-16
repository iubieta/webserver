# ifndef HEADERS_HPP
#define HEADERS_HPP

#include <map>
#include <string>

# define MAX_HEADER_LINE_SIZE 8192
# define MAX_HEADER_SIZE 32768
# define MAX_HEADER_COUNT 100


class Headers
{
	private:

	std::map<std::string, std::string> headers_;
	size_t bytes_header;

	Headers(const Headers &other);
	Headers &operator=(const Headers &other);

	static bool isInvalidKeyCharacter(char c);
	static bool isInvalidValueCharacter(char c);
	static bool isInvalidKey(const std::string &key);
	static bool isInvalidValue(const std::string &value);
	void normalize(std::string &key);
	
	public : 

	Headers();
	~Headers();

	int tokenHeader(const std::string &line);
	int addHeader(const std::string &key, const std::string &value);
	//void setHeader(const std::string &key);

	const std::string *getHeader(const std::string &key) const;
	const std::map<std::string, std::string> &getHeaders() const;
	size_t addBytesHeader(size_t bytes);
	size_t getBytesHeader() const;
	

};

#endif