// http_status_codes.h
// ----------------------------------------------------------------------------

#ifndef HTTP_STATUS_CODES_H

#define HTTP_STATUS_CODES_H

#include <string>
enum e_status_code {
	// Parsing status
	ESTATE_LINE,
	ESTATE_HEADERS,
	ESTATE_BODY,
	ESTATE_COMPLETE,

	// Info
	CONTINUE		= 100,
	SWITCH_PROTOCOL	= 101,

	// Success
	OK				= 200,
	CREATED			= 201,
	NO_CONTENT		= 204,

	// Redirection
	MOVED_PERMANENTLY		= 301,
	FOUND					= 302,
	TEMPORARY_REDIR			= 307,
	PERMANENT_REDIR			= 308,

	// Client Errors
	BAD_REQUEST				= 400,
	FORBIDDEN				= 403,
	NOT_FOUND				= 404,
	METHOD_NOT_ALLOWED		= 405,
	REQUEST_TIMEOUT			= 408,
	LENGTH_REQUIRED			= 411,
	PAYLOAD_TOO_LARGE		= 413,
	URI_TOO_LONG			= 413,
	UNSUPPORTED_MEDIA_TYPE	= 415,
	HEADERS_TOO_LARGE		= 431,

	// Server Errors
	INTERNAL_ERROR			= 500,
	NOT_IMPLEMENTED			= 501,
	SERVICE_UNAVAILABLE		= 503,
	VERSION_NOT_SUPPORTED	= 505
};

typedef struct s_status_message {
	e_status_code code_;
	const char *message_;
} t_status_message;

// TODO: status message table
const std::string getStatusMessage(e_status_code code);

#endif // !HTTP_STATUS_CODES_H
