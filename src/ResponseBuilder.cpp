// ResponseBuilder.cpp
// -----------------------------------------------------------------------------

#include "../inc/ResponseBuilder.hpp"
#include "../inc/file_utils.hpp"
#include "../inc/mime_types.hpp"
#include <cctype>
#include <sstream>
#include <string>

// Helpers ---------------------------------------------------------------------

// Builders --------------------------------------------------------------------
// The builder has the server configs corresponding to the connection as input
ResponseBuilder::ResponseBuilder() {}

ResponseBuilder::~ResponseBuilder() {}

// Private methods -------------------------------------------------------------

// Error Handler
void ResponseBuilder::errorHandler(const Decision& data, HttpResponse &res) const
{
	// If there is a defined error page
	if (!data.getErrorPage().empty())
	{
		std::string content;
		if (loadFile(data.getErrorPage(), content) == FILE_OK)
		{
			res.setBody(content);
			std::string mime_type = getMimeType(getFileExtension(data.getErrorPage()));
			res.addHeader("Content-Type", mime_type);
			return;
		}
	}
	// If the error page is not defined, file opening fails or similar return
	// default body for error pages
	res.setBody(buildDefaultErrorPage(res.getStatusCode()));
	res.addHeader("Content-Type", "text/html");
}

std::string ResponseBuilder::buildDefaultErrorPage(e_status_code code) const
{
	std::ostringstream oss;
	std::string msg = getStatusMessage(code);

    oss << "<!DOCTYPE html>\n"
    << "<html>\n"
    << "<head><title>" << code << " " << msg << "</title></head>\n"
    << "<body>\n"
    << "<h1>" << code << " " << msg << "</h1>\n"
    << "<hr>\n"
    << "<p>webserver/0.1</p>\n"
    << "</body>\n"
    << "</html>\n";

    return oss.str();
}


// Public methods --------------------------------------------------------------

HttpResponse ResponseBuilder::execute(const Decision& data) const {
	HttpResponse res;
	res.setVersion(HTTP_V1_1);
	res.setStatusCode(data.getStatus());
	res.setReasonPhrase(getStatusMessage(data.getStatus()));
	switch (data.getHandler()) {
		case READ_FILE:
			break;
		case WRITE_FILE:
			break;
		case REMOVE_FILE:
			break;
		case LIST_DIR:
			break;
		case ERROR_PAGE:
		{
			errorHandler(data, res);
			break;
		}
		default:
			break;
	}
	return res;
}
