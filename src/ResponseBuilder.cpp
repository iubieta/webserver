// ResponseBuilder.cpp
// -----------------------------------------------------------------------------

#include "../inc/ResponseBuilder.hpp"
#include "../inc/file_utils.hpp"
#include "../inc/mime_types.hpp"
#include <cctype>
#include <sstream>
#include <string>


// Builders --------------------------------------------------------------------
// The builder has the server configs corresponding to the connection as input
ResponseBuilder::ResponseBuilder() {}

ResponseBuilder::~ResponseBuilder() {}

// Helpers ---------------------------------------------------------------------

e_file_result buildBody(const std::string& file_path, HttpResponse& res)
{
	std::string content;
	e_file_result fr = loadFile(file_path, content);
	if (fr == FILE_OK)
	{
		res.setBody(content);
		std::string mime_type = getMimeType(getFileExtension(file_path));
		res.addHeader("Content-Type", mime_type);
	}
	return fr;
}


// Error Handler

void ResponseBuilder::errorHandler(const Decision& data, HttpResponse &res) const
{
	std::string file_path = data.getErrorPage();
	switch (buildBody(file_path, res)) {
		case FILE_OK:
		{
			return;
		}
		default:
		{
			// If the error page is not defined, file opening fails or similar return
			// default body for error pages
			res.setBody(buildDefaultErrorPage(res.getStatusCode()));
			res.addHeader("Content-Type", "text/html");
		}
	}
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
	res.setWholeStatus(data.getStatus());
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
