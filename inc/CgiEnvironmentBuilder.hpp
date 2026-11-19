#ifndef CGI_ENVIRONMENT_BUILDER_HPP
#define CGI_ENVIRONMENT_BUILDER_HPP

#include <string>
#include <vector>

class Request;
class Decision;

// core CGI meta-variables only.
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

    Error build(const Request& request, const Decision& decision);
    const std::vector<std::string>& getVariables() const;
    void clear();

private:
    std::vector<std::string> variables_;
    size_t bytes_;

    CgiEnvironmentBuilder(const CgiEnvironmentBuilder& other);
    CgiEnvironmentBuilder& operator=(const CgiEnvironmentBuilder& other);

    Error add(const std::string& name, const std::string& value);
    static bool validName(const std::string& name);
    static bool validValue(const std::string& value);
};

#endif
