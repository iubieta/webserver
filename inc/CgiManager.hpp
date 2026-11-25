#ifndef CGI_MANAGER_HPP
#define CGI_MANAGER_HPP

#include "CgiProcess.hpp"
#include "CgiLaunchData.hpp"
#include "Request.hpp"
#include "Decision.hpp"
#include <map>
#include <exception>
#include <new>
#include <utility>

// Own CGI processes by stable connection identifier.
class CgiManager {
public:
    CgiManager();
    ~CgiManager();
    enum StartResult {
        CGI_START_STARTED,
        CGI_START_DUPLICATE,
        CGI_START_ENV_ERROR,
        CGI_START_ARGS_ERROR,
        CGI_START_PREPARE_ERROR,
        CGI_START_INVALID,
        CGI_START_PIPE_ERROR,
        CGI_START_FORK_ERROR,
        CGI_START_MEMORY_ERROR
    };
    // Start a registered child; STARTED does not guarantee execve success.
    StartResult start(unsigned long connectionId, const Request& request,
        const Decision& decision, const CgiEnvironmentBuilder::ConnectionInfo& connection);

    CgiProcess* create(unsigned long connectionId, const std::string& body);
    CgiProcess* find(unsigned long connectionId);
    const CgiProcess* find(unsigned long connectionId) const;
    bool remove(unsigned long connectionId);
    std::size_t size() const;
private:
    typedef std::map<unsigned long, CgiProcess*> ProcessMap;
    ProcessMap processes_;
    CgiManager(const CgiManager& other);
    CgiManager& operator=(const CgiManager& other);
};
#endif
