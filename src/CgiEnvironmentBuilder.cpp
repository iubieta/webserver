#include <sstream>
#include <new>

#include "../inc/CgiEnvironmentBuilder.hpp"
#include "../inc/Request.hpp"
#include "../inc/Decision.hpp"
#include "../inc/ServerConfig.hpp"

CgiEnvironmentBuilder::CgiEnvironmentBuilder() : variables_(), bytes_(0) {}
CgiEnvironmentBuilder::~CgiEnvironmentBuilder() {}

void CgiEnvironmentBuilder::clear()
{
    variables_.clear();
    bytes_ = 0;
}

const std::vector<std::string>& CgiEnvironmentBuilder::getVariables() const
{
    return variables_;
}

bool CgiEnvironmentBuilder::validName(const std::string& name)
{
    if (name.empty())
        return false;

    for (size_t i = 0; i < name.size(); ++i) {
        const char c = name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
            return false;
    }
    return true;
}

bool CgiEnvironmentBuilder::validValue(const std::string& value)
{
    return value.find('\0') == std::string::npos
        && value.find('\r') == std::string::npos
        && value.find('\n') == std::string::npos;
}

CgiEnvironmentBuilder::Error CgiEnvironmentBuilder::add(const std::string& name, 
    const std::string& value)
{
    if (!validName(name))
        return CGI_ENV_INVALID_NAME;

    if (!validValue(value))
        return CGI_ENV_INVALID_VALUE;

    for (size_t i = 0; i < variables_.size(); ++i)
        if (variables_[i].compare(0, name.size() + 1, name + "=") == 0)
            return CGI_ENV_NAME_COLLISION;

    // Account for the equals sign and terminating NUL in envp.
    const size_t size = name.size() + 1 + value.size() + 1;
    if (size > 8192 || variables_.size() >= 128 || size > 65536 - bytes_)
        return CGI_ENV_TOO_LARGE;

    variables_.push_back(name + "=" + value);
    bytes_ = bytes_ + size;
    return CGI_ENV_OK;
}

CgiEnvironmentBuilder::Error CgiEnvironmentBuilder::build(const Request& request, 
    const Decision& decision)
{
    // A failed build must never expose entries from a previous request.
    clear();
    try {
        if (decision.getHandler() != CGI || !decision.getServerConfig()
            || decision.getCgiInterpreter().empty()
            || decision.getDiskPath().empty()
            || decision.getCgiScriptName().empty()
            || request.getMethod().empty() || request.getVersion().empty())
            return CGI_ENV_MISSING_VARIABLE;

        const ServerConfig& server = *decision.getServerConfig();
        std::ostringstream port;
        port << server.getListen();
        // Core CGI meta-variables, independent of incoming HTTP_* headers.
        struct Pair { const char* name; std::string value; };
        const Pair vars[] = {
            {"GATEWAY_INTERFACE", "CGI/1.1"},
            {"SERVER_PROTOCOL", request.getVersion()},
            {"REQUEST_METHOD", request.getMethod()},
            {"SCRIPT_NAME", decision.getCgiScriptName()},
            {"QUERY_STRING", request.getQuery()},
            {"SERVER_NAME", server.getServerName()},
            {"SERVER_PORT", port.str()},
            {"SERVER_SOFTWARE", "webserv"}
        };

        for (size_t i = 0; i < sizeof(vars) / sizeof(vars[0]); ++i) {
            const Error err = add(vars[i].name, vars[i].value);
            if (err != CGI_ENV_OK) {
                clear();
                return err;
            }
        }

        // PATH_TRANSLATED is meaningful only when PATH_INFO is present.
        if (!decision.getCgiPathInfo().empty()) {
            Error err = add("PATH_INFO", decision.getCgiPathInfo());
            if (err != CGI_ENV_OK) {
                clear();
                return err;
            }
            if (!decision.getCgiPathTranslated().empty()) {
                err = add("PATH_TRANSLATED", decision.getCgiPathTranslated());
                if (err != CGI_ENV_OK) {
                    clear();
                    return err;
                }
            }
        }

        // CONTENT_LENGTH is derived from the actual decoded request body.
        std::ostringstream length;
        length << request.getBody().size();
        Error err = add("CONTENT_LENGTH", length.str());
        if (err != CGI_ENV_OK) {
            clear();
            return err;
        }
    } catch (const std::bad_alloc&) {
        // Preserve the all-or-nothing contract on allocation failure.
        clear();
        return CGI_ENV_MEMORY_ERROR;
    }
    return CGI_ENV_OK;
}
