// ResponseBuilder.cpp
// -----------------------------------------------------------------------------

#include "../inc/ResponseBuilder.hpp"
#include "../inc/mime_types.hpp"
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>


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
			return;
		case FILE_NOT_FOUND:
			data.setError(NOT_FOUND);
			return errorHandler(data, res);
		case FILE_FORBIDDEN:
			data.setError(FORBIDDEN);
			return errorHandler(data, res);
		case FILE_ERROR:
			data.setError(INTERNAL_ERROR);
			return errorHandler(data, res);
	}
}

// Reading File ----------------------------------------------------------------
void ResponseBuilder::readFile(Decision& data, HttpResponse& res) const {
	std::string file_path = data.getDiskPath();
	e_file_result fr = buildBody(file_path, res);
	if (fr != FILE_OK)
		fileHandlingError(fr, data, res);
}

// Writing file ----------------------------------------------------------------
void ResponseBuilder::writeFile(Decision& data, HttpResponse& res) const {
	std::string file_path = data.getDiskPath();

	// Write file
	bool existed = true;
	const std::string body = data.getBody();
	e_file_result fr = writeToFile(file_path, body, existed);
	if (fr != FILE_OK)
		return fileHandlingError(fr, data, res);
	
	// Set headers
	if (existed) {
		if (body.empty()){
			res.setStatusCode(NO_CONTENT);
		}
	} else {
		res.setStatusCode(CREATED);
		res.addHeader("Location", data.getTarget());
	}
}

// Remove file -----------------------------------------------------------------
void ResponseBuilder::removeFile(Decision& data, HttpResponse& res) const {
	std::string file_path = data.getDiskPath();
	e_file_result fr = removeFileAt(file_path);
	if (fr != FILE_OK) {
		return fileHandlingError(fr, data, res);
	}
	// Set status
	res.setWholeStatus(NO_CONTENT);
}


// List directory --------------------------------------------------------------

// Redirection -----------------------------------------------------------------
void ResponseBuilder::redirect(const Decision& data, HttpResponse &res) const
{
	res.addHeader("Location", data.getRedirURL());
}

// Error Handler ---------------------------------------------------------------
void ResponseBuilder::errorHandler(const Decision& data, HttpResponse &res) const
{
	res.setWholeStatus(data.getStatus());

	std::string file_path = data.getErrorPage();
	if (buildBody(file_path, res) != FILE_OK) {
		res.setBody(buildDefaultErrorPage(res.getStatusCode()));
		res.addHeader("Content-Type", "text/html");
		if (data.getStatus() == METHOD_NOT_ALLOWED) {
			std::vector<std::string> vec = data.getLocationConfig()->getMethods();
			std::string vec_str;
			for (size_t i = 0; i < vec.size(); ++i) {
				vec_str += vec[i];
				if (i < vec.size() - 1)
					vec_str += ", ";
			}
			res.addHeader("Allow", vec_str);
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
			writeFile(data, res);
			break;
		case REMOVE_FILE:
			removeFile(data, res);
			break;
		case LIST_DIR:
			// TODO
			break;
		case REDIRECTION:
			redirect(data, res);
			break;
		case ERROR_PAGE:
			errorHandler(data, res);
			break;
		default:
			break;
	}
	return res;
}
