# ifndef HTTP_RESPONSE
#define HTTP_RESPONSE

#include <map>
#include <string>
#include "http_status_codes.hpp"


class HttpResponse
{
	private:

	e_status_code						status_code_;
	std::string							version_;
	std::string							reason_phrase_;
	std::map<std::string, std::string>	headers_;
	std::string							body_;
	bool								Alive_keep_;

	HttpResponse(const HttpResponse &other);
	HttpResponse &operator=(const HttpResponse &other);

	const std::string  &getCurrentHttpDate() const;

	public:

	HttpResponse();
	~HttpResponse();

	void setStatusCode(e_status_code status_code);
	void setVersion(const std::string &version);
	void setReasonPhrase(const std::string &reason_phrase);
	void setHeaders(const std::map<std::string, std::string> &headers);
	void setBody(const std::string &body);
	void setAliveKeep(bool Alive_keeo);

	int addHeader(const std::string &key, const std::string &vaalue);

	e_status_code getStatusCode() const;
	const std::string &getVersion() const;
	const std::string &getReasonPhrase() const;
	const std::map<std::string, std::string> &getHeaders() const;
	const std::string &getBody() const;
	bool getAliveKeep() const;
	const std::string *getHeader(const std::string &key);

	const std::string &build();

};

#endif