#include "../inc/CgiManager.hpp"

// Start with an empty registry.
CgiManager::CgiManager() : last_token_(0) {}

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
        const CgiProcess::LaunchResult launched = process->launch(executable, data.argv(), data.envp());
        if (launched == CgiProcess::CGI_LAUNCH_STARTED) {
            if (process->inputBuffer().empty())
                closeEmptyInput(connectionId);
            return CGI_START_STARTED;
        }
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
    if (hasEvents(connectionId))
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
        case CGI_STDIN: 
            return process.parentStdinFd();
        case CGI_STDOUT: 
            return process.parentStdoutFd();
        case CGI_EXEC_ERROR: 
            return process.parentExecErrorFd();
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
    if (!process || process->state() != CgiProcess::CGI_RUNNING || process->cleanupStarted())
        return CGI_BIND_INVALID;
    const int fd = channelFd(*process, channel);
    if (fd < 0 || channelDone(*process, channel) || (channel == CGI_STDIN &&
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
        process->cleanupStarted() || channelDone(*process, entry.channel) ||
        channelFd(*process, entry.channel) != entry.fd)
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

// Logical completion is independent from physical descriptor closure.
bool CgiManager::channelDone(const CgiProcess& process, Channel channel) {
    switch (channel) {
        case CGI_STDIN: return process.stdinClosed();
        case CGI_STDOUT: return process.stdoutEof();
        case CGI_EXEC_ERROR: return process.execErrorEof() || process.execErrorReceived();
    }
    return true;
}

// Dispatch at most one I/O operation; HUP on readers must drain buffered data.
CgiManager::DispatchResult CgiManager::dispatchEvent(EventToken token, uint32_t flags) {
    EventBinding binding;
    if (!resolveEvent(token, binding))
        return CGI_DISPATCH_IGNORED;
    CgiProcess* process = find(binding.connectionId);
    CgiProcess::IoResult result = CgiProcess::CGI_IO_INVALID;
    try {
        if (binding.channel == CGI_STDIN) {
            if (!(flags & (EPOLLOUT | EPOLLERR | EPOLLHUP)))
                return CGI_DISPATCH_IGNORED;
            // Read the exec-error stream first to preserve launch failure details.
            if (!process->execErrorEof())
                return CGI_DISPATCH_AGAIN;
            if (flags & (EPOLLERR | EPOLLHUP)) {
                process->fail();
                return CGI_DISPATCH_TERMINAL;
            }
            if (!(flags & EPOLLOUT))
                return CGI_DISPATCH_IGNORED;
            result = process->writeInput(true);
        } else {
            if (!(flags & (EPOLLIN | EPOLLHUP | EPOLLERR)))
                return CGI_DISPATCH_IGNORED;
            result = binding.channel == CGI_STDOUT ? process->readOutput(true)
                : process->readExecError(true);
        }
    } catch (const std::bad_alloc&) {
        process->fail();
        return CGI_DISPATCH_TERMINAL;
    }
    if (result == CgiProcess::CGI_IO_ERROR || result == CgiProcess::CGI_IO_TOO_LARGE ||
        result == CgiProcess::CGI_IO_INVALID) {
        process->fail();
        return CGI_DISPATCH_TERMINAL;
    }
    if (process->state() != CgiProcess::CGI_RUNNING)
        return CGI_DISPATCH_TERMINAL;
    if (channelDone(*process, binding.channel))
        return CGI_DISPATCH_RETIRE;
    if (result == CgiProcess::CGI_IO_AGAIN)
        return CGI_DISPATCH_AGAIN;
    return CGI_DISPATCH_PROGRESS;
}

// Return one binding at a time, including terminal or logically closed channels.
bool CgiManager::nextBinding(unsigned long connectionId, EventToken& token,
    EventBinding& binding) const {
    for (EventMap::const_iterator it = events_.begin(); it != events_.end(); ++it) {
        if (it->second.connectionId == connectionId) {
            token = it->first;
            binding = it->second;
            return true;
        }
    }
    return false;
}

// Caller confirms DEL before this operation; never close a still-active channel.
bool CgiManager::retireEvent(EventToken token) {
    EventMap::iterator it = events_.find(token);
    if (it == events_.end())
        return false;

    const EventBinding binding = it->second;
    CgiProcess* process = find(binding.connectionId);
    if (!process || channelFd(*process, binding.channel) != binding.fd ||
        (process->state() == CgiProcess::CGI_RUNNING && !channelDone(*process, binding.channel)))
        return false;
    // DEL has already succeeded; keep metadata if endpoint validation fails.
    if (!process->closeCompletedEndpoint(binding.fd))
        return false;
    events_.erase(it);
    return true;
}

// No EPOLLOUT registration is needed for an empty or fully sent body.
bool CgiManager::closeEmptyInput(unsigned long connectionId) {
    CgiProcess* process = find(connectionId);
    if (!process || process->state() != CgiProcess::CGI_RUNNING ||
        process->cleanupStarted() || process->inputOffset() != process->inputBuffer().size())
        return false;
    for (EventMap::const_iterator it = events_.begin(); it != events_.end(); ++it) {
        if (it->second.connectionId == connectionId && it->second.channel == CGI_STDIN)
            return false;
    }
    if (process->stdinClosed())
        return process->parentStdinFd() < 0;
    return process->writeInput() == CgiProcess::CGI_IO_CLOSED;
}
