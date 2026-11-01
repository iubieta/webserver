#ifndef REQUEST_LINE_HPP
#define REQUEST_LINE_HPP

#include <string>
#include <vector>
# define MAX_REQUEST_LINE_SIZE 8192


class RequestLine
{
	private:

	std::string method_;
	std::string path_;
	std::string query_;
	std::string version_;
	std::string request_;
	std::string schema_;
	std::string authority_;


	protected:

	RequestLine(const RequestLine &other);
	RequestLine &operator=(const RequestLine &other);
	
	static bool isCharacterInvalid(char c);
	bool isValidMethod(const std::string &method) const;
	bool isVaildVersion(const std::string &version) const;
	bool isValidHexDig(const std::string &str);
	unsigned int hexPairToValue(const std::string &str);
	int	decodePercentEncoding(
	const std::string &old_str, std::string &new_str);
	
	public:

	RequestLine();
	virtual ~RequestLine();

	static int tokenLine(const std::string &line, std::vector<std::string> &token);
 
	virtual int requestLine(const std::vector<std::string> &token) = 0;
	void setRequest(const std::string &request);
	void setMethod(const std::string  &method);
	void setPath(const std::string &path);
	void setVersion(const std::string &version);
	void setQuery(const std::string &query);
	void setSchema(const std::string &schema);
	void setAuthority(const std::string &authority);

	const std::string &getRequest() const;
	const std::string &getMethod() const;
	const std::string &getPath() const;
	const std::string &getVersion() const;
	const std::string &getQuery() const;
	const std::string &getSchema() const;
	const std::string &getAuthority() const;

	bool isImplementedMethod(const std::string &method) const;
	bool isSupportedVersion(const std::string &version) const;

};

#endif