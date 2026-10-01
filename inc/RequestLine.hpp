#ifndef REQUEST_LINE
#define REQUEST_LINE

#include <string>


class RequestLine
{
	private:

	std::string request_;
	std::string method_;
	std::string path_;
	std::string query_;
	std::string version_;

	public:

	RequestLine();
	RequestLine(const RequestLine &other);
	RequestLine &operator=(const RequestLine &other);
	virtual ~RequestLine();
 
	virtual void requestParser(const std::string &buffer) = 0;
	//void setRequest(const std::string &request);
	void setMethod(const std::string  &method);
	void setPath(const std::string &path);
	void setVersion(const std::string &version);

	const std::string &getRequest() const;
	const std::string &getMethod() const;
	const std::string &getPath() const;
	const std::string &getVersion() const;


};

#endif