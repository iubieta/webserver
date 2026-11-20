#ifndef CGI_EXEC_ARGUMENTS_HPP
#define CGI_EXEC_ARGUMENTS_HPP

#include <string>
#include <vector>
#include <new>

// Own the strings and pointer arrays passed to execve (C++98).
// Do not modify or destroy this object while execve uses its pointers.
class CgiExecArguments {
public:
    enum Error {
        CGI_ARGS_OK,
        CGI_ARGS_MISSING,
        CGI_ARGS_INVALID_QUERY,
        CGI_ARGS_TOO_LARGE,
        CGI_ARGS_MEMORY_ERROR
    };

    CgiExecArguments();
    ~CgiExecArguments();

    // Build interpreter argv and copy validated NAME=value environment entries.
    // Indexed-query words are added only for GET without an unencoded '='.
    Error build(const std::string& interpreter, const std::string& scriptPath,
        const std::string& method, const std::string& rawQuery,
        const std::vector<std::string>& environment);

    // Return NULL-terminated arrays, valid until clear(), build() or destruction.
    char* const* argv() const;
    char* const* envp() const;
    const std::vector<std::string>& arguments() const;
    void clear();

private:
    std::vector<std::string> arguments_;
    std::vector<std::string> environment_;
    std::vector<char*> argv_;
    std::vector<char*> envp_;

    CgiExecArguments(const CgiExecArguments& other);
    CgiExecArguments& operator=(const CgiExecArguments& other);

    // Decode one application/x-www-form-urlencoded indexed-query word.
    static bool decodeWord(const std::string& input, std::string& output);
    // Reject invalid, empty or oversized entries before creating pointer arrays.
    static bool validEnvironment(const std::vector<std::string>& entries);
    // Rebuild pointer arrays only after all owned strings are finalized.
    void rebuildPointers();
};

#endif
