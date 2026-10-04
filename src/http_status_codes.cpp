// http_status_codes.cpp
// ----------------------------------------------------------------------------

#include "../inc/http_status_codes.hpp"
#include <iostream>

static const  t_status_message status_messages[] = 
{
	{ CONTINUE ,				"Continue" },
	{ SWITCH_PROTOCOL,			"Switching Protocols" },
	{ OK ,						"OK" },
	{ CREATED ,					"Created" },
	{ NO_CONTENT ,				"No Content" },
	{ MOVED_PERMANENTLY ,		"Moved Permanently" },
	{ FOUND ,					"Found" },
	{ TEMPORARY_REDIR ,			"Temporary Redirect" },
	{ PERMANENT_REDIR , 		"Permanent Redirect" },
	{ BAD_REQUEST ,				"Bad Request" },
	{ FORBIDDEN , 				"Forbidden" },
	{ NOT_FOUND , 				"Not Found" },
	{ METHOD_NOT_ALLOWED , 		"Mehtod Not Allowed" },
	{ REQUEST_TIMEOUT , 		"Request Timeout" },
	{ LENGTH_REQUIRED , 		"Length Required" },
	{ PAYLOAD_TOO_LARGE , 		"Payload Too Large" },
	{ UNSUPPORTED_MEDIA_TYPE , 	"Unsupported Media Type" },
	{ HEADERS_TOO_LARGE , 		"Request Header Field Too Large" },
	{ INTERNAL_ERROR , 			"Internal Server Error" },
	{ NOT_IMPLEMENTED , 		"Not Implemented" },
	{ SERVICE_UNAVAILABLE , 	"Service Unavailable" },
	{ VERSION_NOT_SUPPORTED , 	"HTTP Version Not Supported" },

};

const std::string getStatusMessage(e_status_code code)
{
	for (size_t i = 0; i < sizeof(status_messages) / sizeof(t_status_message); ++i)
	{
		if (code == status_messages[i].code_)
			return status_messages[i].message_;
	}
	return "UNKNOWN ERROR";
}

// int main() {
// 	std::cout << getStatusMessage(NOT_FOUND) << std::endl;
// 	std::cout << getStatusMessage(VERSION_NOT_SUPPORTED) << std::endl;
// 	std::cout << getStatusMessage(OK) << std::endl;
// }

