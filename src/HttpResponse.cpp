#include "../inc/HttpResponse.hpp"
#include "../inc/log_global.hpp"
#include <ctime>
#include <sstream>

HttpResponse::HttpResponse():
			status_code_(OK),
			version_("HTTP/1.1"),
			reason_phrase_("ok"),
			headers_(),
			body_(""),
			Alive_keep_(false)
			{}

HttpResponse:: ~HttpResponse() {}

void HttpResponse::setStatusCode(e_status_code status_code)
{
	status_code_ = status_code;
}

void HttpResponse::setVersion(const std::string &version)
{
	version_ = version;
}

void HttpResponse::setReasonPhrase(const std::string &reason_phrase)
{
	reason_phrase_ = reason_phrase;
}

void HttpResponse::setHeaders(const std::map<std::string, std::string> &headers)
{
	headers_ = headers;
}

void HttpResponse::setBody(const std::string &body)
{
	body_ = body;
}

void HttpResponse::setAliveKeep(bool Alive_keep)
{
	Alive_keep_ = Alive_keep;
}

int HttpResponse::addHeader(const std::string &key, const std::string &value)
{
	if(headers_.count(key) > 0)
	{
		ft_log::global().debug("header: duplicate field");
		return -1;
	}

	headers_.insert(std::make_pair(key, value));
	return 0;
}

e_status_code HttpResponse::getStatusCode() const
{
	return status_code_;
}

const std::string &HttpResponse::getVersion() const
{
	return version_;
}

const std::string &HttpResponse::getReasonPhrase() const
{
	return reason_phrase_;
}

const std::map<std::string, std::string> &HttpResponse::getHeaders() const
{
	return headers_;
}

const std::string &HttpResponse::getBody() const
{
	return body_;
}

bool HttpResponse::getAliveKeep() const
{
	return Alive_keep_;
}

const std::string *HttpResponse::getHeader(const std::string &key)
{
	std::map<std::string, std::string>::const_iterator it = headers_.find(key);

	if (it != headers_.end())
		return &(it->second);
	return NULL;
}

const std::string &HttpResponse::getCurrentHttpDate() const
{
    char buffer[100];
    
    // Retrieve the current calendar time of the system as a std::time_t value
    time_t now = std::time(NULL);
    
    //Convert the absolute time value into a broken-down tm structure in UTC/GMT
    struct tm* gmt = std::gmtime(&now);
    
    if (gmt != NULL) 
	{
        //"Day, DD Mon YYYY HH:MM:SS GMT" (e.g., "Sun, 06 Nov 1994 08:49:37 GMT")
        std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
        return std::string(buffer);
    }
    
    //Return an empty string as a safe fallback if std::gmtime returns NULL
    return "";
}

const std::string &HttpResponse::build()
{
	std::stringstream response;
	

	// Status line
	response << version_ <<" "<< status_code_<<" "<< reason_phrase_<<"\r\n";

	//Header fields
	response <<"Date: "<< getCurrentHttpDate()<<"\r\n";
	response <<"Server: "<< "webserver/1.0\r\n";

	//Empty line
	response << "\r\n";

	//Body

	return response.str();
}