# ifndef HTTP_RESPONSE
#define HTTP_RESPONSE

#include <map>
#include <string>
#include "http_status_codes.hpp"

#ifndef DEF_SERVER_NAME 
# define DEF_SERVER_NAME "webserver/0.1"
#endif // !DEF_SERVER_NAME 

class HttpResponse
{
	private:

	std::string							version_;
	e_status_code						status_code_;
	std::string							reason_phrase_;

	std::map<std::string, std::string>	headers_;
	bool								keep_alive_;

	std::string							body_;

	HttpResponse(const HttpResponse &other);
	HttpResponse &operator=(const HttpResponse &other);

	std::string  getCurrentHttpDate() const;
	std::string  sizetoString(size_t size);
	bool	mayHasBody() const;

	public:

	HttpResponse();
	~HttpResponse();

	void setStatusCode(e_status_code status_code);
	void setVersion(const std::string &version);
	void setReasonPhrase(const std::string &reason_phrase);
	void setHeaders(const std::map<std::string, std::string> &headers);
	void setBody(const std::string &body);
	void setAliveKeep(bool keep_alive);

	int addHeader(const std::string &key, const std::string &vaalue);

	e_status_code getStatusCode() const;
	const std::string &getVersion() const;
	const std::string &getReasonPhrase() const;
	const std::map<std::string, std::string> &getHeaders() const;
	const std::string &getBody() const;
	bool getAliveKeep() const;
	const std::string *getHeader(const std::string &key) const;

	std::string build();

};

#endif
