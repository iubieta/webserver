#include "../inc/CgiExecArguments.hpp"

CgiExecArguments::CgiExecArguments() : arguments_(), environment_(), argv_(), envp_() {}
CgiExecArguments::~CgiExecArguments() {}

// Reset owned storage and invalidate any pointers previously returned.
void CgiExecArguments::clear() {
    argv_.clear();
    envp_.clear();
    arguments_.clear();
    environment_.clear();
}

// Convert one hexadecimal digit; -1 signals malformed percent encoding.
static int hexDigit(char c) {
    if (c >= '0' && c <= '9') 
        return c - '0';
    if (c >= 'a' && c <= 'f') 
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') 
        return c - 'A' + 10;
    return -1;
}

// Decode percent escapes and '+' without accepting embedded NUL bytes.
bool CgiExecArguments::decodeWord(const std::string& input, std::string& output) {
    output.clear();
    for (size_t i = 0; i < input.size(); ++i) {
        char c = input[i];
        if (c == '%') {
            if (i + 2 >= input.size()) 
                return false;
            const int hi = hexDigit(input[i + 1]);
            const int lo = hexDigit(input[i + 2]);
            if (hi < 0 || lo < 0) 
                return false;
            c = static_cast<char>(hi * 16 + lo);
            i += 2;
        } 
        else if (c == '+') {
            c = ' ';
        }
        if (c == '\0') 
            return false;
        output += c;
    }
    return true;
}

// Enforce the environment limits independently of its producing builder.
bool CgiExecArguments::validEnvironment(const std::vector<std::string>& entries) {
    if (entries.size() > 128) 
        return false;

    size_t total = 0;
    for (size_t i = 0; i < entries.size(); ++i) {
        const std::string& entry = entries[i];
        const size_t eq = entry.find('=');
        if (eq == std::string::npos || eq == 0 || entry.find('\0') != std::string::npos
            || entry.size() + 1 > 8192 || entry.size() + 1 > 65536 - total)
            return false;
        for (size_t j = 0; j < eq; ++j) {
            const char c = entry[j];
            if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'))
                return false;
        }
        for (size_t j = eq + 1; j < entry.size(); ++j) {
            if (entry[j] == '\r' || entry[j] == '\n') 
                return false;
        }
        for (size_t k = 0; k < i; ++k) {
            if (entries[k].compare(0, eq + 1, entry, 0, eq + 1) == 0)
                return false;
        }
        total += entry.size() + 1;
    }
    return true;
}

// Populate C pointer arrays only after vector<string> has stopped changing.
void CgiExecArguments::rebuildPointers() {
    argv_.reserve(arguments_.size() + 1);
    envp_.reserve(environment_.size() + 1);

    for (size_t i = 0; i < arguments_.size(); ++i)
        argv_.push_back(const_cast<char*>(arguments_[i].c_str()));
    argv_.push_back(NULL);
    for (size_t i = 0; i < environment_.size(); ++i)
        envp_.push_back(const_cast<char*>(environment_[i].c_str()));
    envp_.push_back(NULL);
}

// Prepare argv[0]=interpreter, argv[1]=physical script and optional query words.
// No partial result is exposed if validation or allocation fails.
CgiExecArguments::Error CgiExecArguments::build(
    const std::string& interpreter, const std::string& scriptPath,
    const std::string& method, const std::string& rawQuery,
    const std::vector<std::string>& environment) {
    clear();
    if (interpreter.empty() || scriptPath.empty() || method.empty()
        || interpreter.find('\0') != std::string::npos
        || scriptPath.find('\0') != std::string::npos)
        return CGI_ARGS_MISSING;
    if (!validEnvironment(environment)) 
        return CGI_ARGS_TOO_LARGE;

    try {
        arguments_.push_back(interpreter);
        arguments_.push_back(scriptPath);
        // RFC 3875 indexed queries: only when there is no raw '=' delimiter.
        if (method == "GET" && !rawQuery.empty() && rawQuery.find('=') == std::string::npos) {
            size_t start = 0;
            while (true) {
                const size_t end = rawQuery.find('+', start);
                const std::string part = rawQuery.substr(start, end == std::string::npos
                    ? std::string::npos : end - start);
                std::string word;
                if (!decodeWord(part, word)) { 
                    clear(); 
                    return CGI_ARGS_INVALID_QUERY; 
                }
                // Limit query-derived arguments independently of environment limits.
                if (arguments_.size() >= 130 || word.size() > 8192) {
                    clear(); 
                    return CGI_ARGS_TOO_LARGE;
                }
                arguments_.push_back(word);
                if (end == std::string::npos) 
                    break;
                start = end + 1;
            }
        }
        environment_ = environment;
        rebuildPointers();
    } catch (const std::bad_alloc&) {
        clear();
        return CGI_ARGS_MEMORY_ERROR;
    }
    return CGI_ARGS_OK;
}

char* const* CgiExecArguments::argv() const { 
    return argv_.empty() ? NULL : &argv_[0]; 
}

char* const* CgiExecArguments::envp() const { 
    return envp_.empty() ? NULL : &envp_[0]; 
}

const std::vector<std::string>& CgiExecArguments::arguments() const { 
    return arguments_; 
}
