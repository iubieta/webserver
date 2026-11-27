#include "../inc/CgiManager.hpp"

// Start with an empty registry.
CgiManager::CgiManager() : last_token_(0), shutting_down_(false) {}

// Shutdown must resolve every child before destroying the manager.
CgiManager::~CgiManager() {
    while (!processes_.empty()) {
        if (!remove(processes_.begin()->first))
            std::terminate();
    }
}

// Allocate and prepare a process before publishing it in the registry.
CgiProcess* CgiManager::create(unsigned long connectionId, const std::string& body) {
    if (shutting_down_ || find(connectionId) != NULL)
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
        Supervision info = {0, 0, false, false, false, CGI_RESULT_FAILED, false};
        supervision_.insert(std::make_pair(connectionId, info));
    } catch (...) {
        processes_.erase(connectionId);
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
    if (shutting_down_)
        return CGI_START_SHUTTING_DOWN;
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
            const std::time_t now = std::time(NULL);
            Supervision& info = supervision_.find(connectionId)->second;
            if (now != static_cast<std::time_t>(-1)) {
                info.started = now;
                info.timed = true;
            } else {
                info.failure = CGI_RESULT_SUPERVISION_ERROR;
                process->fail();
            }
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
    supervision_.erase(connectionId);
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
    if (shutting_down_ || token == 0)
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
        (!process->execErrorEof() || process->execErrorReceived() ||
         process->inputOffset() >= process->inputBuffer().size())))
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

// Fail only a running execution; preserve an existing terminal outcome.
bool CgiManager::failRegistration(unsigned long connectionId) {
    CgiProcess* process = find(connectionId);
    if (!process || process->state() != CgiProcess::CGI_RUNNING)
        return false;
    supervision_.find(connectionId)->second.failure = CGI_RESULT_REGISTRATION_ERROR;
    process->fail();
    return true;
}

// The caller must establish that ADD did not install a kernel registration.
bool CgiManager::registrationFailed(EventToken token) {
    EventMap::iterator it = events_.find(token);
    if (it == events_.end())
        return false;
    failRegistration(it->second.connectionId);
    events_.erase(it);
    return true;
}

#ifdef CGI_INTERNAL_TESTS
// Remove metadata after epoll DEL, even if the process is no longer running.
bool CgiManager::unbindEvent(EventToken token) {
    return events_.erase(token) != 0;
}
#endif

// Keep completed bindings discoverable even after a failed DEL attempt.
bool CgiManager::nextRetirement(EventToken& token, EventBinding& binding) const {
    for (EventMap::const_iterator it = events_.begin(); it != events_.end(); ++it) {
        const CgiProcess* process = find(it->second.connectionId);
        if (process && (process->state() != CgiProcess::CGI_RUNNING ||
            channelDone(*process, it->second.channel))) {
            token = it->first;
            binding = it->second;
            return true;
        }
    }
    return false;
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
        case CGI_STDIN: 
            return process.stdinClosed();
        case CGI_STDOUT: 
            return process.stdoutEof();
        case CGI_EXEC_ERROR: 
            return process.execErrorEof() || process.execErrorReceived();
    }
    return true;
}

// Dispatch at most one I/O operation; HUP on readers must drain buffered data.
CgiManager::DispatchResult CgiManager::dispatchEvent(EventToken token, uint32_t flags) {
    EventBinding binding;
    if (!resolveEvent(token, binding))
        return CGI_DISPATCH_IGNORED;
    CgiProcess* process = find(binding.connectionId);
    Supervision& info = supervision_.find(binding.connectionId)->second;
    CgiProcess::IoResult result = CgiProcess::CGI_IO_INVALID;
    try {
        if (binding.channel == CGI_STDIN) {
            if (!(flags & (EPOLLOUT | EPOLLERR | EPOLLHUP)))
                return CGI_DISPATCH_IGNORED;
            // Read the exec-error stream first to preserve launch failure details.
            if (!process->execErrorEof())
                return CGI_DISPATCH_AGAIN;
            if (flags & (EPOLLERR | EPOLLHUP)) {
                info.failure = CGI_RESULT_IO_ERROR;
                process->fail();
                return CGI_DISPATCH_TERMINAL;
            }
            if (!(flags & EPOLLOUT))
                return CGI_DISPATCH_IGNORED;
            result = process->writeInput(true);
        } else {
            if (!(flags & (EPOLLIN | EPOLLHUP | EPOLLERR)))
                return CGI_DISPATCH_IGNORED;
            if (binding.channel == CGI_STDOUT) {
                const std::size_t used = bufferedOutputBytes();
                const std::size_t remaining = used >= MAX_GLOBAL_OUTPUT_BYTES ? 0
                    : MAX_GLOBAL_OUTPUT_BYTES - used;
                result = process->readOutput(true, remaining);
            } else
                result = process->readExecError(true);
        }
    } catch (const std::bad_alloc&) {
        info.failure = CGI_RESULT_MEMORY_ERROR;
        process->fail();
        return CGI_DISPATCH_TERMINAL;
    }
    if (result == CgiProcess::CGI_IO_ERROR || result == CgiProcess::CGI_IO_TOO_LARGE ||
        result == CgiProcess::CGI_IO_INVALID || result == CgiProcess::CGI_IO_GLOBAL_LIMIT) {
        info.failure = result == CgiProcess::CGI_IO_TOO_LARGE ? CGI_RESULT_OUTPUT_LIMIT
            : result == CgiProcess::CGI_IO_GLOBAL_LIMIT ? CGI_RESULT_GLOBAL_LIMIT
            : CGI_RESULT_IO_ERROR;
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

// Cancel a live execution; never overwrite a terminal result.
bool CgiManager::cancel(unsigned long connectionId) {
    CgiProcess* process = find(connectionId);
    if (!process || process->state() != CgiProcess::CGI_RUNNING)
        return false;
    process->cancel();
    return true;
}

// Expose terminal bindings even when resolveEvent rejects them.
bool CgiManager::nextTerminalBinding(EventToken& token, EventBinding& binding) const {
    for (EventMap::const_iterator it = events_.begin(); it != events_.end(); ++it) {
        const CgiProcess* process = find(it->second.connectionId);
        if (process && process->state() != CgiProcess::CGI_RUNNING) {
            token = it->first;
            binding = it->second;
            return true;
        }
    }
    return false;
}

// A result remains owned by the manager until explicit removal.
bool CgiManager::ready(unsigned long connectionId) const {
    const CgiProcess* p = find(connectionId);
    return p && p->cleanupStarted() && p->state() != CgiProcess::CGI_RUNNING &&
        p->state() != CgiProcess::CGI_CREATED &&
        (p->pid() <= 0 || p->childExited()) && !hasEvents(connectionId);
}

// Poll first, preserve terminal states, and never close registered endpoints.
CgiManager::SupervisionResult CgiManager::superviseOne(CgiProcess& p,
    Supervision& info, std::time_t now) {
    if (p.state() == CgiProcess::CGI_CREATED)
        return CGI_SUPERVISION_OK;
    if (!info.timed) {
        info.started = now;
        info.timed = true;
    }
    if (p.pid() > 0 && !p.childExited()) {
        const CgiProcess::WaitResult r = p.pollChild();
        if (r == CgiProcess::CGI_WAIT_ERROR || r == CgiProcess::CGI_WAIT_INVALID) {
            if (p.state() == CgiProcess::CGI_RUNNING)
                info.failure = CGI_RESULT_SUPERVISION_ERROR;
            p.fail();
            return CGI_SUPERVISION_ERROR;
        }
        if (r == CgiProcess::CGI_WAIT_INTERRUPTED)
            return CGI_SUPERVISION_OK;
    }
    if (p.state() == CgiProcess::CGI_RUNNING &&
        std::difftime(now, info.started) >= TIMEOUT_SECONDS)
        p.timeOut();
    if (p.state() == CgiProcess::CGI_RUNNING)
        return CGI_SUPERVISION_OK;
    p.discardFailedOutput();
    if (hasEvents(p.connectionId()))
        return CGI_SUPERVISION_RETIRE;
    if (!info.termSent) {
        const CgiProcess::CleanupResult r = p.beginCleanup();
        if (r == CgiProcess::CGI_CLEANUP_ERROR)
            return CGI_SUPERVISION_ERROR;
        if (r == CgiProcess::CGI_CLEANUP_INTERRUPTED)
            return CGI_SUPERVISION_OK;
        info.termSent = true;
        info.termAt = now;
    }
    if (!p.childExited() && p.pid() > 0 && !info.killSent && 
            std::difftime(now, info.termAt) >= GRACE_SECONDS) {
        const CgiProcess::SignalResult r = p.forceKill();
        if (r == CgiProcess::CGI_SIGNAL_ERROR || r == CgiProcess::CGI_SIGNAL_INVALID)
            return CGI_SUPERVISION_ERROR;
        info.killSent = true;
    }
    return CGI_SUPERVISION_OK;
}

// Visit every child even if another child requires retirement or reports an error.
CgiManager::SupervisionResult CgiManager::supervise() {
    const std::time_t now = std::time(NULL);
    if (now == static_cast<std::time_t>(-1))
        return CGI_SUPERVISION_ERROR;
    SupervisionResult result = CGI_SUPERVISION_OK;
    for (ProcessMap::iterator it = processes_.begin(); it != processes_.end(); ++it) {
        const SupervisionResult r = superviseOne(*it->second,
            supervision_.find(it->first)->second, now);
        if (r == CGI_SUPERVISION_ERROR || (r == CGI_SUPERVISION_RETIRE && result == CGI_SUPERVISION_OK))
            result = r;
    }
    return result;
}

// Count actual stored stdout, including results not yet consumed.
std::size_t CgiManager::bufferedOutputBytes() const {
    std::size_t total = 0;
    for (ProcessMap::const_iterator it = processes_.begin(); it != processes_.end(); ++it) {
        const std::size_t n = it->second->outputBuffer().size();
        if (n > static_cast<std::size_t>(-1) - total)
            return static_cast<std::size_t>(-1);
        total += n;
    }
    return total;
}

// Find one completed execution without exposing its process pointer.
bool CgiManager::nextReady(unsigned long& connectionId) const {
    if (shutting_down_)
        return false;

    for (ProcessMap::const_iterator it = processes_.begin(); it != processes_.end(); ++it) {
        if (!supervision_.find(it->first)->second.detached && ready(it->first)) {
            connectionId = it->first;
            return true;
        }
    }
    return false;
}

// Deliver raw stdout only on successful execution, then release the process.
bool CgiManager::takeResult(unsigned long connectionId, CgiResult& result) {
    if (!result.output.empty() || !ready(connectionId) || 
        supervision_.find(connectionId)->second.detached || shutting_down_)
        return false;
    CgiProcess* p = find(connectionId);
    ResultStatus status = supervision_.find(connectionId)->second.failure;
    if (p->state() == CgiProcess::CGI_TIMED_OUT)
        status = CGI_RESULT_TIMEOUT;
    else if (p->state() == CgiProcess::CGI_CANCELLED)
        status = CGI_RESULT_CANCELLED;
    else if (p->execErrorReceived())
        status = CGI_RESULT_EXEC_ERROR;
    else if (p->state() == CgiProcess::CGI_FINISHED)
        status = p->childExited() && WIFEXITED(p->childExitStatus()) &&
            WEXITSTATUS(p->childExitStatus()) == 0 ? CGI_RESULT_SUCCESS : CGI_RESULT_CHILD_ERROR;
    CgiResult pending;
    if (status == CGI_RESULT_SUCCESS && !p->takeOutput(pending.output))
        return false;
    pending.connectionId = connectionId;
    pending.status = status;
    pending.childExited = p->childExited();
    pending.childExitStatus = p->childExitStatus();
    pending.execError = p->execErrorReceived() ? p->execErrorCode() : 0;
    if (!remove(connectionId))
        return false;
    result.output.swap(pending.output);
    result.connectionId = pending.connectionId;
    result.status = pending.status;
    result.childExited = pending.childExited;
    result.childExitStatus = pending.childExitStatus;
    result.execError = pending.execError;
    return true;
}

// Expose only the descriptor needed for event registration.
int CgiManager::endpointFd(unsigned long connectionId, Channel channel) const {
    const CgiProcess* p = find(connectionId);
    if (!p || shutting_down_ || p->state() != CgiProcess::CGI_RUNNING ||
        channelDone(*p, channel))
        return -1;
    return channelFd(*p, channel);
}

// Freeze admission and preserve already terminal outcomes.
void CgiManager::beginShutdown() {
    shutting_down_ = true;
    for (ProcessMap::iterator it = processes_.begin(); it != processes_.end(); ++it)
        if (it->second->state() == CgiProcess::CGI_RUNNING)
            it->second->cancel();
}

// Never block or remove a registered endpoint; the event loop performs DEL.
CgiManager::SupervisionResult CgiManager::shutdownStep() {
    if (!shutting_down_)
        return CGI_SUPERVISION_ERROR;
    const SupervisionResult result = supervise();
    ProcessMap::iterator it = processes_.begin();
    while (it != processes_.end()) {
        const unsigned long id = it->first;
        ++it;
        if (ready(id) || find(id)->state() == CgiProcess::CGI_CREATED)
            remove(id);
    }
    return result;
}

// Destruction is safe only after all owned children and bindings are gone.
bool CgiManager::shutdownComplete() const {
    return shutting_down_ && processes_.empty() && events_.empty();
}

// Offer only live, unbound endpoints that can make useful I/O progress.
bool CgiManager::pendingInterest(const CgiProcess& process, Channel channel,
    EventInterest& interest) const {
    if (shutting_down_ || process.state() != CgiProcess::CGI_RUNNING ||
        process.cleanupStarted() || channelDone(process, channel))
        return false;
    const int fd = channelFd(process, channel);
    if (fd < 0)
        return false;
    // Delay writable interest until the exec-error stream has closed cleanly.
    if (channel == CGI_STDIN && (!process.execErrorEof() ||
        process.execErrorReceived() ||
        process.inputOffset() >= process.inputBuffer().size()))
        return false;
    for (EventMap::const_iterator it = events_.begin(); it != events_.end(); ++it) {
        if (it->second.fd == fd || (it->second.connectionId == process.connectionId() &&
            it->second.channel == channel))
            return false;
    }
    EventInterest pending = {process.connectionId(), channel, fd,
        static_cast<uint32_t>(channel == CGI_STDIN ? EPOLLOUT : EPOLLIN)};
    interest = pending;
    return true;
}

// Enumerate pending interests without allocating tokens or modifying epoll.
bool CgiManager::nextPendingInterest(EventInterest& interest) const {
    if (shutting_down_)
        return false;
    for (ProcessMap::const_iterator it = processes_.begin(); it != processes_.end(); ++it) {
        const CgiProcess& process = *it->second;
        if (pendingInterest(process, CGI_STDOUT, interest) ||
            pendingInterest(process, CGI_EXEC_ERROR, interest) ||
            pendingInterest(process, CGI_STDIN, interest))
            return true;
    }
    return false;
}

// Keep the identifier reserved until the child and all endpoints are released.
bool CgiManager::contains(unsigned long connectionId) const {
    return find(connectionId) != NULL;
}

// Disconnect is independent of the child's terminal outcome.
bool CgiManager::detach(unsigned long connectionId) {
    CgiProcess* process = find(connectionId);
    if (!process)
        return false;
    supervision_.find(connectionId)->second.detached = true;
    if (process->state() == CgiProcess::CGI_RUNNING)
        process->cancel();
    return true;
}

// Drive lifecycle work even on an empty or interrupted event batch.
CgiManager::SupervisionResult CgiManager::tick() {
    if (shutting_down_)
        return shutdownStep();
    SupervisionResult result = supervise();
    ProcessMap::iterator it = processes_.begin();
    while (it != processes_.end()) {
        const unsigned long id = it->first;
        ++it;
        if (supervision_.find(id)->second.detached &&
            (ready(id) || find(id)->state() == CgiProcess::CGI_CREATED)) {
            if (!remove(id))
                result = CGI_SUPERVISION_ERROR;
        }
    }
    // Completed channels of running children also require retirement.
    EventToken token;
    EventBinding binding;
    if (result == CGI_SUPERVISION_OK && nextRetirement(token, binding))
        result = CGI_SUPERVISION_RETIRE;
    return result;
}
