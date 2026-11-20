#include "../inc/CgiEnvironmentBuilder.hpp"

CgiEnvironmentBuilder::CgiEnvironmentBuilder() : variables_(), bytes_(0) {}
CgiEnvironmentBuilder::~CgiEnvironmentBuilder() {}

// Reset both stored entries and their accumulated byte count.
void CgiEnvironmentBuilder::clear()
{
    variables_.clear();
    bytes_ = 0;
}

const std::vector<std::string>& CgiEnvironmentBuilder::getVariables() const
{
    return variables_;
}

// Require a portable, unambiguous CGI variable name.
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

// Avoid embedded NUL and line breaks in exported metadata.
bool CgiEnvironmentBuilder::validValue(const std::string& value)
{
    return value.find('\0') == std::string::npos
        && value.find('\r') == std::string::npos
        && value.find('\n') == std::string::npos;
}

// Validate a single entry before it is committed to the environment.
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

// Convert an HTTP field name to its CGI HTTP_* environment name.
// Reject non-token characters rather than silently changing their meaning.
std::string CgiEnvironmentBuilder::headerVariableName(const std::string& header) {
    if (header.empty()) 
        return "";

    std::string name = "HTTP_";
    for (size_t i = 0; i < header.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(header[i]);
        if (c >= 'a' && c <= 'z') 
            name += static_cast<char>(c - 'a' + 'A');
        else if (c >= 'A' && c <= 'Z') 
            name += static_cast<char>(c);
        else if (c >= '0' && c <= '9') 
            name += static_cast<char>(c);
        else if (c == '-') 
            name += '_';
        else 
            return "";
    }
    return name;
}

// Export request headers without forwarding hop-by-hop or credential fields.
// Request currently stores one value per key; no duplicate merging is attempted.
CgiEnvironmentBuilder::Error CgiEnvironmentBuilder::addHeaders(const Request& request) {
    const std::map<std::string, std::string>& headers = request.getHeaders();

    // Connection can nominate additional hop-by-hop headers. Reject such
    // requests here until a complete connection-token filter is available.
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string key = it->first;
        for (size_t i = 0; i < key.size(); ++i) {
            if (key[i] >= 'A' && key[i] <= 'Z') 
                key[i] = key[i] - 'A' + 'a';
        }
        if (key == "connection" && !it->second.empty()) {
            std::string value = it->second;
            for (size_t i = 0; i < value.size(); ++i) {
                if (value[i] >= 'A' && value[i] <= 'Z') 
                    value[i] = value[i] - 'A' + 'a';
            }
            // Accept only simple known values until a complete token parser
            // can safely suppress all nominated hop-by-hop fields.
            if (value != "keep-alive" && value != "close")
                return CGI_ENV_INVALID_VALUE;
        }
    }

    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string lower = it->first;
        for (size_t i = 0; i < lower.size(); ++i) {
            const unsigned char c = static_cast<unsigned char>(lower[i]);
            if (c >= 'A' && c <= 'Z') 
                lower[i] = static_cast<char>(c - 'A' + 'a');
        }
        // These fields have dedicated CGI variables or are not forwarded.
        if (lower == "content-length" || lower == "content-type" ||
            lower == "authorization" || lower == "proxy-authorization" ||
            lower == "connection" || lower == "proxy-connection" ||
            lower == "keep-alive" || lower == "transfer-encoding" ||
            lower == "te" || lower == "trailer" || lower == "upgrade")
            continue;
        const std::string name = headerVariableName(lower);
        if (name.empty()) 
            return CGI_ENV_INVALID_NAME;
        const Error err = add(name, it->second);
        if (err != CGI_ENV_OK) 
            return err;
    }

    // CONTENT_TYPE is not prefixed with HTTP_ in CGI.
    std::map<std::string, std::string>::const_iterator found = headers.end();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
        std::string key = it->first;
        for (size_t i = 0; i < key.size(); ++i) {
            const unsigned char c = static_cast<unsigned char>(key[i]);
            if (c >= 'A' && c <= 'Z') 
                key[i] = static_cast<char>(c - 'A' + 'a');
        }
        if (key == "content-type") {
            if (found != headers.end()) 
                return CGI_ENV_NAME_COLLISION;
            found = it;
        }
    }
    if (found != headers.end()) 
        return add("CONTENT_TYPE", found->second);
    return CGI_ENV_OK;
}

// Build all CGI metadata or leave an empty environment on any failure.
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
        err = addHeaders(request);
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

// Extend the validated core environment with trusted socket metadata.
// Reject missing peer addresses instead of fabricating them from Host/X-Forwarded-For.
CgiEnvironmentBuilder::Error CgiEnvironmentBuilder::build( const Request& request, 
    const Decision& decision, const ConnectionInfo& connection) {
    if (connection.remoteAddress.empty()) {
        clear();
        return CGI_ENV_MISSING_VARIABLE;
    }

    const Error base = build(request, decision);
    if (base != CGI_ENV_OK) 
        return base;
    try {
        Error err = add("REMOTE_ADDR", connection.remoteAddress);
        if (err == CGI_ENV_OK && !connection.remoteHost.empty())
            err = add("REMOTE_HOST", connection.remoteHost);
        if (err != CGI_ENV_OK) {
            clear();
            return err;
        }
    } catch (const std::bad_alloc&) {
        clear();
        return CGI_ENV_MEMORY_ERROR;
    }
    return CGI_ENV_OK;
}
