#ifndef CGI_MANAGER_HPP
#define CGI_MANAGER_HPP

#include "CgiProcess.hpp"
#include "CgiLaunchData.hpp"
#include "Request.hpp"
#include "Decision.hpp"
#include <map>
#include <stdint.h>
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

    typedef uint64_t EventToken;

    enum Channel { 
        CGI_STDIN, 
        CGI_STDOUT, 
        CGI_EXEC_ERROR 
    };

    struct EventBinding {
        unsigned long connectionId;
        int fd;
        Channel channel;
    };

    enum BindResult {
        CGI_BIND_OK, 
        CGI_BIND_INVALID, 
        CGI_BIND_DUPLICATE,
        CGI_BIND_STALE_TOKEN, 
        CGI_BIND_MEMORY_ERROR
    };

    // Bind a live parent endpoint to a globally allocated, increasing token.
    BindResult bindEvent(EventToken token, unsigned long connectionId, Channel channel);
    // Resolve only a live binding; copy metadata, never transfer ownership.
    bool resolveEvent(EventToken token, EventBinding& binding) const;
    // Retire after epoll DEL; never close the descriptor here.
    bool unbindEvent(EventToken token);
    std::size_t eventCount() const;

    CgiProcess* create(unsigned long connectionId, const std::string& body);
    CgiProcess* find(unsigned long connectionId);
    const CgiProcess* find(unsigned long connectionId) const;
    bool remove(unsigned long connectionId);
    std::size_t size() const;
private:
    typedef std::map<unsigned long, CgiProcess*> ProcessMap;
    ProcessMap processes_;
    typedef std::map<EventToken, EventBinding> EventMap;
    EventMap events_;
    EventToken last_token_;

    static int channelFd(const CgiProcess& process, Channel channel);
    bool hasEvents(unsigned long connectionId) const;
    CgiManager(const CgiManager& other);
    CgiManager& operator=(const CgiManager& other);
};
#endif
