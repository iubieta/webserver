#ifndef CGI_MANAGER_HPP
#define CGI_MANAGER_HPP

#include "CgiProcess.hpp"
#include "CgiLaunchData.hpp"
#include "Request.hpp"
#include "Decision.hpp"
#include <map>
#include <ctime>
#include <stdint.h>
#include <exception>
#include <new>
#include <utility>
#include <sys/epoll.h>

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
        CGI_START_MEMORY_ERROR,
        CGI_START_SHUTTING_DOWN
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

    // Describe an eligible endpoint before the event loop assigns a token.
    struct EventInterest {
        unsigned long connectionId;
        Channel channel;
        int fd;
        uint32_t flags;
    };
    // Return one unbound interest; leave the destination unchanged on failure.
    bool nextPendingInterest(EventInterest& interest) const;

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

    enum DispatchResult {
        CGI_DISPATCH_IGNORED, 
        CGI_DISPATCH_AGAIN, 
        CGI_DISPATCH_PROGRESS,
        CGI_DISPATCH_RETIRE, 
        CGI_DISPATCH_TERMINAL
    };

    // Dispatch one bounded nonblocking operation using Linux readiness flags.
    DispatchResult dispatchEvent(EventToken token, uint32_t flags);
    // Find even inactive bindings for teardown; outputs change only on success.
    bool nextBinding(unsigned long connectionId, EventToken& token, EventBinding& binding) const;
    // After successful epoll DEL, retire metadata and close an inactive endpoint.
    bool retireEvent(EventToken token);
    // Close unregistered stdin when no body bytes remain.
    bool closeEmptyInput(unsigned long connectionId);

    // Periodic supervision; caller must wake even without pipe events.
    enum SupervisionResult { 
        CGI_SUPERVISION_OK, 
        CGI_SUPERVISION_RETIRE,
        CGI_SUPERVISION_ERROR 
    };

    SupervisionResult supervise();
    bool cancel(unsigned long connectionId);
    // Return a terminal connection with bindings still requiring epoll DEL.
    bool nextTerminalBinding(EventToken& token, EventBinding& binding) const;
    bool ready(unsigned long connectionId) const;
    static const unsigned int TIMEOUT_SECONDS = 30;
    static const unsigned int GRACE_SECONDS = 2;

    enum ResultStatus { 
        CGI_RESULT_SUCCESS, 
        CGI_RESULT_EXEC_ERROR,
        CGI_RESULT_IO_ERROR, 
        CGI_RESULT_OUTPUT_LIMIT, 
        CGI_RESULT_GLOBAL_LIMIT,
        CGI_RESULT_MEMORY_ERROR, 
        CGI_RESULT_SUPERVISION_ERROR,
        CGI_RESULT_CHILD_ERROR, 
        CGI_RESULT_TIMEOUT, 
        CGI_RESULT_CANCELLED,
        CGI_RESULT_FAILED 
    };
    struct CgiResult {
        unsigned long connectionId;
        ResultStatus status;
        bool childExited;
        int childExitStatus;
        int execError;
        std::string output;
    };

    static const std::size_t MAX_GLOBAL_OUTPUT_BYTES = 100u * 1024u * 1024u;
    std::size_t bufferedOutputBytes() const;
    bool nextReady(unsigned long& connectionId) const;
    // Consume a ready result; destination output must be empty.
    bool takeResult(unsigned long connectionId, CgiResult& result);

    // Stop accepting work and cancel live children; caller retires epoll bindings.
    void beginShutdown();
    // Drive nonblocking cleanup and discard completed results during shutdown.
    SupervisionResult shutdownStep();
    bool shutdownComplete() const;
    // Obtain a parent endpoint without exposing mutable process ownership.
    int endpointFd(unsigned long connectionId, Channel channel) const;

#ifdef CGI_INTERNAL_TESTS
public:
#else
private:
#endif
    CgiProcess* create(unsigned long connectionId, const std::string& body);
    CgiProcess* find(unsigned long connectionId);
    const CgiProcess* find(unsigned long connectionId) const;
    bool remove(unsigned long connectionId);
public:
    std::size_t size() const;
private:
    typedef std::map<unsigned long, CgiProcess*> ProcessMap;
    ProcessMap processes_;
    typedef std::map<EventToken, EventBinding> EventMap;
    EventMap events_;
    EventToken last_token_;
    bool shutting_down_;
    struct Supervision {
        std::time_t started;
        std::time_t termAt;
        bool timed;
        bool termSent;
        bool killSent;
        ResultStatus failure;
    };
    typedef std::map<unsigned long, Supervision> SupervisionMap;
    SupervisionMap supervision_;

    SupervisionResult superviseOne(CgiProcess& process, Supervision& info, std::time_t now);
    bool pendingInterest(const CgiProcess& process, Channel channel, EventInterest& interest) const;
    static int channelFd(const CgiProcess& process, Channel channel);
    bool hasEvents(unsigned long connectionId) const;
    static bool channelDone(const CgiProcess& process, Channel channel);
    CgiManager(const CgiManager& other);
    CgiManager& operator=(const CgiManager& other);
};
#endif
