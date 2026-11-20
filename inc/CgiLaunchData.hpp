#ifndef CGI_LAUNCH_DATA_HPP
#define CGI_LAUNCH_DATA_HPP

#include "CgiEnvironmentBuilder.hpp"
#include "CgiExecArguments.hpp"
#include "Request.hpp"
#include "Decision.hpp"

class Request;
class Decision;

// Prepare the complete CGI environment and execve arrays without forking.
// Own the storage until the future CgiProcess invokes execve.
class CgiLaunchData {
public:
    enum Error {
        CGI_LAUNCH_OK,
        CGI_LAUNCH_ENV_ERROR,
        CGI_LAUNCH_ARGS_ERROR
    };

    CgiLaunchData();
    ~CgiLaunchData();

    // Build both stages atomically: a failure clears all launch data.
    Error build(const Request& request, const Decision& decision,
        const CgiEnvironmentBuilder::ConnectionInfo& connection);

    // Return execve-compatible arrays; pointers live until clear/build/destruction.
    char* const* argv() const;
    char* const* envp() const;
    const std::vector<std::string>& arguments() const;
    const std::vector<std::string>& environment() const;
    CgiEnvironmentBuilder::Error environmentError() const;
    CgiExecArguments::Error argumentsError() const;
    void clear();

private:
    CgiEnvironmentBuilder environmentBuilder_;
    CgiExecArguments execArguments_;
    CgiEnvironmentBuilder::Error environmentError_;
    CgiExecArguments::Error argumentsError_;

    CgiLaunchData(const CgiLaunchData& other);
    CgiLaunchData& operator=(const CgiLaunchData& other);
};

#endif
