#ifndef CGI_ENVIRONMENT_BUILDER_HPP
#define CGI_ENVIRONMENT_BUILDER_HPP

#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <new>

#include "Request.hpp"
#include "Decision.hpp"
#include "ServerConfig.hpp"

class Request;
class Decision;

// Build a bounded, validated CGI environment from a parsed HTTP request.
class CgiEnvironmentBuilder {
public:
    enum Error {
        CGI_ENV_OK,
        CGI_ENV_MISSING_VARIABLE,
        CGI_ENV_INVALID_NAME,
        CGI_ENV_INVALID_VALUE,
        CGI_ENV_NAME_COLLISION,
        CGI_ENV_TOO_LARGE,
        CGI_ENV_MEMORY_ERROR
    };

    CgiEnvironmentBuilder();
    ~CgiEnvironmentBuilder();

    // Build the CGI environment from the parsed request and routing decision.
    Error build(const Request& request, const Decision& decision);
    // Return NAME=value entries; conversion to envp happens later.
    const std::vector<std::string>& getVariables() const;
    // Discard all entries and reset the byte budget.
    void clear();

private:
    std::vector<std::string> variables_;
    size_t bytes_;

    CgiEnvironmentBuilder(const CgiEnvironmentBuilder& other);
    CgiEnvironmentBuilder& operator=(const CgiEnvironmentBuilder& other);

    // Append a validated NAME=value entry without duplicates or budget overflow.
    // On error the caller discards the entire environment.
    Error add(const std::string& name, const std::string& value);
    // Accept only uppercase ASCII letters, digits and underscores.
    static bool validName(const std::string& name);
    // Reject NUL and line breaks in CGI metadata values.
    static std::string headerVariableName(const std::string& header);
    // Export HTTP fields conservatively; reject ambiguous CGI name collisions.
    Error addHeaders(const Request& request);
    static bool validValue(const std::string& value);
};

#endif
