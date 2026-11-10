// ResponseBuilder.cpp
// -----------------------------------------------------------------------------

#include "../inc/ResponseBuilder.hpp"
#include "../inc/mime_types.hpp"
#include <cctype>
#include <sstream>
#include <string>


// Builders --------------------------------------------------------------------
// The builder has the server configs corresponding to the connection as input
ResponseBuilder::ResponseBuilder() {}

ResponseBuilder::~ResponseBuilder() {}

// Helpers ---------------------------------------------------------------------
e_file_result ResponseBuilder::buildBody
	(const std::string& file_path, HttpResponse& res) const
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

void ResponseBuilder::fileHandlingError
	(e_file_result error, Decision& data, HttpResponse& res) const
{
	switch (error) {
		case FILE_OK:
		{
			return;
		}
		case FILE_NOT_FOUND:
		{
			data.setError(NOT_FOUND);
			res.setWholeStatus(NOT_FOUND);
			errorHandler(data, res);
			return;
		}
		case FILE_FORBIDDEN:
		{
			data.setError(FORBIDDEN);
			res.setWholeStatus(FORBIDDEN);
			errorHandler(data, res);
			return;
		}
		case FILE_ERROR:
		{
			data.setError(INTERNAL_ERROR);
			res.setWholeStatus(INTERNAL_ERROR);
			errorHandler(data, res);
			return;
		}
	}
}

// Reading File ----------------------------------------------------------------
void ResponseBuilder::readFile(Decision& data, HttpResponse& res) const {
	std::string file_path = data.getDiskPath();
	e_file_result fr = buildBody(file_path, res);
	if (fr == FILE_OK)
		return;
	else {
		fileHandlingError(fr, data, res);
	}
}

// Writing file ----------------------------------------------------------------
void ResponseBuilder::writeFile(Decision& data, HttpResponse& res) const {
	std::string file_path = data.getDiskPath();
	e_file_result fr = writeToFile(file_path, data.body);
	if (fr == FILE_OK)
		return;
	else {
		fileHandlingError(fr, data, res);
	}
}

// Remove file -----------------------------------------------------------------

// List directory --------------------------------------------------------------

// Redirection -----------------------------------------------------------------

// Error Handler ---------------------------------------------------------------
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
			// If the error page is not defined, file opening fails 
			// or similar return default body for error pages
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

HttpResponse ResponseBuilder::execute(Decision& data) const {
	HttpResponse res;
	res.setVersion(HTTP_V1_1);
	res.setWholeStatus(data.getStatus());
	switch (data.getHandler()) {
		case READ_FILE:
			readFile(data, res);
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
