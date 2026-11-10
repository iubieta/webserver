// ResponseBuilder.hpp
// -----------------------------------------------------------------------------

#ifndef RESPONSE_BUILDER_HPP
# define RESPONSE_BUILDER_HPP

#include "Decision.hpp"
#include "HttpResponse.hpp"
#include "http_status_codes.hpp"

class ResponseBuilder {
	private:
		ResponseBuilder(const ResponseBuilder& other);
		ResponseBuilder& operator=(const ResponseBuilder& other);

		// Handlers
		void readFile(Decision& data, HttpResponse &res) const;
		void writeFile(HttpResponse &res) const;
		void removeFile(HttpResponse &res) const;
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
