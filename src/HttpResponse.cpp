#include "../inc/HttpResponse.hpp"
#include "../inc/log_global.hpp"
#include "../inc/Headers.hpp"
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
	std::string key_temp = key;
	Headers::normalize(key_temp);
	if(headers_.count(key_temp) > 0)
	{
		ft_log::global().debug("header: duplicate field");
		return -1;
	}
	headers_.insert(std::make_pair(key_temp, value));
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

const std::string *HttpResponse::getHeader(const std::string &key) const
{
	std::string key_temp = key;
	std::map<std::string, std::string>::const_iterator it;

	Headers::normalize(key_temp);
	it = headers_.find(key_temp);
	
	if (it != headers_.end())
		return &(it->second);
	return NULL;
}

std::string HttpResponse::sizetoString(size_t size)
{
	std::string answer = "";

    if (size >= 10) 
        answer += sizetoString(size / 10);
	
	answer += static_cast<unsigned char>('0' + (size % 10));

    return answer;
}

std::string HttpResponse::getCurrentHttpDate() const
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

bool HttpResponse::mayHasBody() const
{
	if ((status_code_ >= 100 && status_code_ <= 199)
		|| status_code_ != NO_CONTENT)
		return false;
	
	return true;
}

std::string HttpResponse::build()
{
	std::stringstream response;
	std::map<std::string, std::string>::const_iterator it;
	
	reason_phrase_ = getStatusMessage(status_code_);

	// Status line
	response << version_ <<" "<< status_code_<<" "<< reason_phrase_<<"\r\n";

	//Header fields
	if(!getHeader("date"))
		response <<"date: "<< getCurrentHttpDate()<<"\r\n";
	if(!getHeader("server"))
		response <<"server: webserver/1.0\r\n";
	
	// Prohibido en 1xx y 204, incluso con valor cero.
	if (mayHasBody())
	{
		if(!getHeader("content-length"))
			response << "content-length: "<< body_.size() << "\r\n";
		else
		{
			headers_["content-length"] = sizetoString(body_.size());
		}
	}
	

	//Allow	Obligatorio en una respuesta 405. "Method Not Allowed"
	//Tipo MIME (std::string contentType): Identificador del contenido (ej. "text/html", "image/png", "application/json").

	if(!getHeader("Connection"))
		 response << "Connection: " << (Alive_keep_? "Keep-Alive" : "close") << "\r\n";

	it = headers_.begin();
	for(; it != headers_.end(); ++it)
	{
		if(it->first == "content-length" && !mayHasBody())
			continue;
		response << it->first << ": "<< it ->second << "\r\n";
	}

	//Empty line
	response << "\r\n";

	//Body
	if (mayHasBody() && !body_.empty())
		response << body_;

	return response.str();
}


// #include <iostream>

// int main()
// {
// 	HttpResponse	ok_response;
// 	HttpResponse	error_response;
// 	HttpResponse	empty_response;

// 	// Respuesta con body y longitud manual incorrecta.
// 	// build() debe sustituir 100 por 4.
// 	ok_response.setStatusCode(CONTINUE);
// 	ok_response.setBody("Hola");
// 	ok_response.addHeader("Content-Type", "text/plain");
// 	ok_response.addHeader("Content-Length", "100");
// 	ok_response.setAliveKeep(false);

// 	std::cout << "===== 200 CON BODY =====" << std::endl;
// 	std::cout << ok_response.build();
// 	std::cout << "\n\n";

// 	// Respuesta de error con contenido HTML.
// 	error_response.setStatusCode(NOT_FOUND);
// 	error_response.setBody("<h1>404 Not Found</h1>");
// 	error_response.addHeader("Content-Type", "text/html");
// 	error_response.setAliveKeep(false);

// 	std::cout << "===== 404 CON BODY =====" << std::endl;
// 	std::cout << error_response.build();
// 	std::cout << "\n\n";

// 	// Respuesta correcta sin contenido.
// 	empty_response.setStatusCode(OK);
// 	empty_response.setAliveKeep(false);

// 	std::cout << "===== 200 SIN BODY =====" << std::endl;
// 	std::cout << empty_response.build();
// 	std::cout << "\n";

// 	return (0);
// }