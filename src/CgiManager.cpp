#include "../inc/CgiManager.hpp"

// Start with an empty registry.
CgiManager::CgiManager() {}

// Shutdown must resolve every child before destroying the manager.
CgiManager::~CgiManager() {
    while (!processes_.empty()) {
        if (!remove(processes_.begin()->first))
            std::terminate();
    }
}

// Allocate and prepare a process before publishing it in the registry.
CgiProcess* CgiManager::create(unsigned long connectionId, const std::string& body) {
    if (find(connectionId) != NULL)
        return NULL;
    CgiProcess* process = new CgiProcess();
    try {
        if (!process->prepare(connectionId, body)) {
            delete process;
            return NULL;
        }
        if (!processes_.insert(std::make_pair(connectionId, process)).second) {
            delete process;
            return NULL;
        }
    } catch (...) {
        delete process;
        throw;
    }
    return process;
}

// Build launch data before registration and retain ownership before forking.
// Bind events only after a successful launch, so pre-fork failure can roll back.
CgiManager::StartResult CgiManager::start(unsigned long connectionId,
    const Request& request, const Decision& decision,
    const CgiEnvironmentBuilder::ConnectionInfo& connection) {
    if (find(connectionId) != NULL)
        return CGI_START_DUPLICATE;
    try {
        CgiLaunchData data;
        const CgiLaunchData::Error built = data.build(request, decision, connection);
        if (built == CgiLaunchData::CGI_LAUNCH_ENV_ERROR) {
            if (data.environmentError() == CgiEnvironmentBuilder::CGI_ENV_MEMORY_ERROR)
                return CGI_START_MEMORY_ERROR;
            return CGI_START_ENV_ERROR;
        }
        if (built == CgiLaunchData::CGI_LAUNCH_ARGS_ERROR) {
            if (data.argumentsError() == CgiExecArguments::CGI_ARGS_MEMORY_ERROR)
                return CGI_START_MEMORY_ERROR;
            return CGI_START_ARGS_ERROR;
        }
        // Resolve the executable reference before publishing the process.
        const std::string executable = decision.getCgiInterpreter();
        CgiProcess* process = create(connectionId, request.getBody());
        if (process == NULL)
            return CGI_START_PREPARE_ERROR;
        const CgiProcess::LaunchResult launched = process->launch(
            executable, data.argv(), data.envp());
        if (launched == CgiProcess::CGI_LAUNCH_STARTED)
            return CGI_START_STARTED;
        // All current launch failures occur before a child is owned.
        remove(connectionId);
        if (launched == CgiProcess::CGI_LAUNCH_PIPE_ERROR)
            return CGI_START_PIPE_ERROR;
        if (launched == CgiProcess::CGI_LAUNCH_FORK_ERROR)
            return CGI_START_FORK_ERROR;
        return CGI_START_INVALID;
    } catch (const std::bad_alloc&) {
        return CGI_START_MEMORY_ERROR;
    }
}

// Return a borrowed pointer, never transfer ownership.
CgiProcess* CgiManager::find(unsigned long connectionId) {
    ProcessMap::iterator it = processes_.find(connectionId);
    return it == processes_.end() ? NULL : it->second;
}

// Provide read-only lookup for a const manager.
const CgiProcess* CgiManager::find(unsigned long connectionId) const {
    ProcessMap::const_iterator it = processes_.find(connectionId);
    return it == processes_.end() ? NULL : it->second;
}

// Refuse removal until child supervision and descriptor cleanup are resolved.
// Future event registrations must also be removed before calling this method.
bool CgiManager::remove(unsigned long connectionId) {
    ProcessMap::iterator it = processes_.find(connectionId);
    if (it == processes_.end())
        return false;
    CgiProcess* process = it->second;
    if (process->pid() > 0 && !process->childExited())
        return false;
    if (process->state() == CgiProcess::CGI_RUNNING)
        return false;
    process->closePipes();
    delete process;
    processes_.erase(it);
    return true;
}

// Count registered processes, including those awaiting cleanup.
std::size_t CgiManager::size() const {
    return processes_.size();
}

// Return the current parent endpoint for a channel, or -1 for an invalid channel.
int CgiManager::channelFd(const CgiProcess& process, Channel channel) {
    switch (channel) {
        case CGI_STDIN: return process.parentStdinFd();
        case CGI_STDOUT: return process.parentStdoutFd();
        case CGI_EXEC_ERROR: return process.parentExecErrorFd();
    }
    return -1;
}

// Reserve one association per live endpoint; the caller performs epoll ADD.
CgiManager::BindResult CgiManager::bindEvent(EventToken token,
    unsigned long connectionId, Channel channel) {
    if (token == 0)
        return CGI_BIND_INVALID;
    if (events_.find(token) != events_.end())
        return CGI_BIND_DUPLICATE;
    if (token <= last_token_)
        return CGI_BIND_STALE_TOKEN;
    const CgiProcess* process = find(connectionId);
    if (!process || process->state() != CgiProcess::CGI_RUNNING ||
        process->cleanupStarted())
        return CGI_BIND_INVALID;
    const int fd = channelFd(*process, channel);
    if (fd < 0 || (channel == CGI_STDIN &&
        process->inputOffset() >= process->inputBuffer().size()))
        return CGI_BIND_INVALID;
    for (EventMap::const_iterator it = events_.begin(); it != events_.end(); ++it) {
        if (it->second.fd == fd || (it->second.connectionId == connectionId &&
            it->second.channel == channel))
            return CGI_BIND_DUPLICATE;
    }
    EventBinding binding = {connectionId, fd, channel};
    try {
        if (!events_.insert(std::make_pair(token, binding)).second)
            return CGI_BIND_DUPLICATE;
    } catch (const std::bad_alloc&) {
        return CGI_BIND_MEMORY_ERROR;
    }
    last_token_ = token;
    return CGI_BIND_OK;
}

// Reject retired tokens and endpoints that have already been closed.
bool CgiManager::resolveEvent(EventToken token, EventBinding& binding) const {
    EventMap::const_iterator it = events_.find(token);
    if (it == events_.end())
        return false;
    const EventBinding& entry = it->second;
    const CgiProcess* process = find(entry.connectionId);
    if (!process || process->state() != CgiProcess::CGI_RUNNING ||
        process->cleanupStarted() || channelFd(*process, entry.channel) != entry.fd)
        return false;
    binding = entry;
    return true;
}

// Remove metadata after epoll DEL, even if the process is no longer running.
bool CgiManager::unbindEvent(EventToken token) {
    return events_.erase(token) != 0;
}

// Count associations, including those awaiting explicit retirement.
std::size_t CgiManager::eventCount() const {
    return events_.size();
}

// Prevent destruction while an event association still references this process.
bool CgiManager::hasEvents(unsigned long connectionId) const {
    for (EventMap::const_iterator it = events_.begin(); it != events_.end(); ++it) {
        if (it->second.connectionId == connectionId)
            return true;
    }
    return false;
}
