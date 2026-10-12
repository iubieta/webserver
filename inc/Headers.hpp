# ifndef HEADERS_HPP
#define HEADERS_HPP

#include <map>
#include <string>


class Headers
{
	private:

	std::map<std::string, std::string> headers_;

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

	

};

#endif