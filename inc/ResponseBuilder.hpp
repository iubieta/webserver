// ResponseBuilder.hpp
// -----------------------------------------------------------------------------

#ifndef RESPONSE_BUILDER_HPP
# define RESPONSE_BUILDER_HPP

#include "Decision.hpp"
#include "HttpResponse.hpp"
#include "http_status_codes.hpp"
#include "file_utils.hpp"

class ResponseBuilder {
	private:
		ResponseBuilder(const ResponseBuilder& other);
		ResponseBuilder& operator=(const ResponseBuilder& other);

		// Helpers
		e_file_result buildBody(const std::string& file_path, HttpResponse& res) const;
		void fileHandlingError(e_file_result error, Decision& data, HttpResponse& res) const;

		// Handlers
		void readFile(Decision& data, HttpResponse &res) const;
		void writeFile(Decision& data, HttpResponse &res) const;
		void removeFile(Decision& data, HttpResponse &res) const;
		void listDir(HttpResponse &res) const;
		void redirect(HttpResponse &res) const;

		// Error Handler
		void errorHandler(const Decision& data, HttpResponse &res) const;
		std::string buildDefaultErrorPage(e_status_code code) const;

	public:
		ResponseBuilder();
		~ResponseBuilder();

		HttpResponse execute(Decision& data) const;
};

#endif // !RESPONSE_BUILDER_HPP
