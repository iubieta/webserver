#include "../inc/CgiProcess.hpp"

CgiProcess::CgiProcess()
    : state_(CGI_CREATED), pid_(-1), connection_id_(0),
      stdin_closed_(false), stdout_eof_(false), child_exited_(false),
      child_exit_status_(0), input_offset_(0), exec_error_offset_(0),
      exec_error_received_(false), exec_error_eof_(false), exec_error_code_(0),
      cleanup_started_(false) {
    stdin_pipe_[0] = stdin_pipe_[1] = -1;
    stdout_pipe_[0] = stdout_pipe_[1] = -1;
    exec_error_pipe_[0] = exec_error_pipe_[1] = -1;
}

CgiProcess::~CgiProcess() { 
    closePipes(); 
}

// Retry close after EINTR is unsafe because the descriptor may already be reused.
void CgiProcess::closeFd(int& fd) {
    if (fd >= 0) {
        const int old = fd;
        fd = -1;
        close(old);
    }
}

// Preserve existing file status flags while enabling nonblocking I/O.
bool CgiProcess::setNonBlocking(int fd) {
    const int flags = fcntl(fd, F_GETFL);
    return flags != -1 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

// Close this descriptor automatically when a child successfully executes a program.
bool CgiProcess::setCloseOnExec(int fd) {
    const int flags = fcntl(fd, F_GETFD);
    return flags != -1 && fcntl(fd, F_SETFD, flags | FD_CLOEXEC) != -1;
}

// Create stdin, stdout, and exec-error pipes atomically from the caller's view.
// Parent uses stdin[1], stdout[0], exec_error[0]; the child uses opposite ends.
bool CgiProcess::createPipes() {
    if (cleanup_started_ || state_ != CGI_CREATED || stdin_pipe_[0] >= 0 || stdout_pipe_[0] >= 0 ||
        exec_error_pipe_[0] >= 0)
        return false;

    if (pipe(stdin_pipe_) == -1 || pipe(stdout_pipe_) == -1 ||
        pipe(exec_error_pipe_) == -1 ||
        !setNonBlocking(stdin_pipe_[1]) ||
        !setNonBlocking(stdout_pipe_[0]) ||
        !setNonBlocking(exec_error_pipe_[0])) {
        closePipes();
        return false;
    }

    // All original endpoints must disappear from unrelated executed children.
    if (!setCloseOnExec(stdin_pipe_[0]) || !setCloseOnExec(stdin_pipe_[1]) ||
        !setCloseOnExec(stdout_pipe_[0]) || !setCloseOnExec(stdout_pipe_[1]) ||
        !setCloseOnExec(exec_error_pipe_[0]) || !setCloseOnExec(exec_error_pipe_[1])) {
        closePipes();
        return false;
    }
    return true;
}

// The parent must not retain child-only ends, otherwise EOF can be delayed.
void CgiProcess::closeChildEndsInParent() {
    closeFd(stdin_pipe_[0]);
    closeFd(stdout_pipe_[1]);
    closeFd(exec_error_pipe_[1]);
}

// The forked child must not retain the parent ends.
void CgiProcess::closeParentEndsInChild() {
    closeFd(stdin_pipe_[1]);
    closeFd(stdout_pipe_[0]);
    closeFd(exec_error_pipe_[0]);
}

// Safe to invoke repeatedly, including after partial pipe creation.
void CgiProcess::closePipes() {
    closeFd(stdin_pipe_[0]); 
    closeFd(stdin_pipe_[1]);
    closeFd(stdout_pipe_[0]); 
    closeFd(stdout_pipe_[1]);
    closeFd(exec_error_pipe_[0]); 
    closeFd(exec_error_pipe_[1]);
}


// Replace the initial body only while the process has not started.
bool CgiProcess::prepare(unsigned long connectionId, const std::string& requestBody) {
    if (cleanup_started_ || state_ != CGI_CREATED)
        return false;
    input_buffer_ = requestBody;
    connection_id_ = connectionId;
    input_offset_ = 0;
    return true;
}

// Mark a successfully forked child; PID zero is not a valid child PID.
bool CgiProcess::markRunning(pid_t childPid) {
    if (cleanup_started_ || state_ != CGI_CREATED || childPid <= 0)
        return false;
    pid_ = childPid;
    state_ = CGI_RUNNING;
    return true;
}

// A reaped child is not sufficient to finish until stdout also reaches EOF.
void CgiProcess::markChildExited(int waitStatus) {
    if (pid_ <= 0 || child_exited_)
        return;
    child_exited_ = true;
    child_exit_status_ = waitStatus;
    tryFinish();
}

// Preserve any pending output until the stream has been fully consumed.
void CgiProcess::markStdoutEof() {
    if (state_ != CGI_RUNNING)
        return;
    stdout_eof_ = true;
    tryFinish();
}

// stdin closure is tracked separately and does not determine completion.
void CgiProcess::markStdinClosed() {
    if (state_ == CGI_CREATED || state_ == CGI_RUNNING)
        stdin_closed_ = true;
}

// Use subtraction to avoid overflow when enforcing the 10 MiB limit.
CgiProcess::OutputResult CgiProcess::appendOutput(const char* bytes, std::size_t length) {
    if (state_ != CGI_RUNNING || stdout_eof_ || (bytes == NULL && length != 0))
        return CGI_OUTPUT_INVALID_STATE;
    if (length > MAX_OUTPUT_BYTES - output_buffer_.size())
        return CGI_OUTPUT_TOO_LARGE;
    if (length != 0)
        output_buffer_.append(bytes, length);
    return CGI_OUTPUT_OK;
}

// Record a terminal transport failure; resource cleanup comes in later steps.
void CgiProcess::fail() {
    if (!terminal()) 
        state_ = CGI_FAILED;
}

// Record a terminal timeout without overwriting an earlier terminal state.
void CgiProcess::timeOut() {
    if (!terminal()) 
        state_ = CGI_TIMED_OUT;
}

// Record a disconnected client without overwriting an earlier terminal state.
void CgiProcess::cancel() {
    if (!terminal()) 
        state_ = CGI_CANCELLED;
}

// Advance only by the number of bytes confirmed written to the stdin pipe.
bool CgiProcess::advanceInput(std::size_t count) {
    if (state_ != CGI_RUNNING || stdin_closed_ || count > input_buffer_.size() - input_offset_)
        return false;
    input_offset_ = input_offset_ + count;
    return true;
}

bool CgiProcess::terminal() const {
    return state_ == CGI_FINISHED || state_ == CGI_FAILED ||
           state_ == CGI_TIMED_OUT || state_ == CGI_CANCELLED;
}

// Only both child reaping and stdout EOF can complete a running process.
void CgiProcess::tryFinish() {
    if (state_ == CGI_RUNNING && child_exited_ && stdout_eof_ &&
        exec_error_eof_ && !exec_error_received_ && exec_error_offset_ == 0)
        state_ = CGI_FINISHED;
}

// Launch a child after creating all pipes. The parent retains only its ends.
// No std::string/vector operations or allocations are performed in the child.
CgiProcess::LaunchResult CgiProcess::launch(const std::string& executable,
    char* const* argv, char* const* envp) {
    if (state_ != CGI_CREATED || executable.empty() ||
        executable.find('\0') != std::string::npos ||
        argv == NULL || argv[0] == NULL || envp == NULL ||
        stdin_pipe_[0] >= 0 || stdin_pipe_[1] >= 0 ||
        stdout_pipe_[0] >= 0 || stdout_pipe_[1] >= 0 ||
        exec_error_pipe_[0] >= 0 || exec_error_pipe_[1] >= 0)
        return CGI_LAUNCH_INVALID;

    if (!createPipes())
        return CGI_LAUNCH_PIPE_ERROR;

    const pid_t child = fork();
    if (child < 0) {
        closePipes();
        return CGI_LAUNCH_FORK_ERROR;
    }
    if (child == 0) {
        closeParentEndsInChild();
        // Duplicate source descriptors above stdio before dup2: this also
        // handles a server launched with stdin or stdout initially closed.
        int input = fcntl(stdin_pipe_[0], F_DUPFD, 3);
        int output = fcntl(stdout_pipe_[1], F_DUPFD, 3);
        int errorFd = fcntl(exec_error_pipe_[1], F_DUPFD, 3);
        int failure = errno;
        if (input < 0 || output < 0 || errorFd < 0) {
            if (errorFd >= 0) 
                write(errorFd, &failure, sizeof(failure));
            else 
                write(exec_error_pipe_[1], &failure, sizeof(failure));
            std::terminate();
        }
        closeFd(stdin_pipe_[0]);
        closeFd(stdout_pipe_[1]);
        closeFd(exec_error_pipe_[1]);
        // F_DUPFD does not preserve FD_CLOEXEC; restore it for the error pipe.
        const int flags = fcntl(errorFd, F_GETFD);
        if (flags == -1 || fcntl(errorFd, F_SETFD, flags | FD_CLOEXEC) == -1 ||
            dup2(input, STDIN_FILENO) == -1 ||
            dup2(output, STDOUT_FILENO) == -1) {
            failure = errno;
            write(errorFd, &failure, sizeof(failure));
            std::terminate();
        }
        close(input);
        close(output);
        execve(executable.c_str(), argv, envp);
        failure = errno;
        write(errorFd, &failure, sizeof(failure));
        std::terminate();
    }
    closeChildEndsInParent();
    markRunning(child);
    return CGI_LAUNCH_STARTED;
}

// Write at most one chunk; progress is recorded only after a successful write.
// SIGPIPE must be ignored or otherwise handled by the hosting server.
CgiProcess::IoResult CgiProcess::writeInput() {
    return writeInput(false);
}

// Transfer one chunk with optional deferred descriptor closure.
CgiProcess::IoResult CgiProcess::writeInput(bool deferClose) {
    if (state_ != CGI_RUNNING || stdin_closed_ || stdin_pipe_[1] < 0)
        return CGI_IO_INVALID;
    if (input_offset_ == input_buffer_.size()) {
        if (!deferClose)
            closeFd(stdin_pipe_[1]);
        markStdinClosed();
        return CGI_IO_CLOSED;
    }

    const std::size_t remaining = input_buffer_.size() - input_offset_;
    const std::size_t count = remaining < 16384u ? remaining : 16384u;
    const ssize_t n = write(stdin_pipe_[1], input_buffer_.data() + input_offset_, count);
    if (n > 0) {
        input_offset_ = input_offset_ + static_cast<std::size_t>(n);
        if (input_offset_ == input_buffer_.size()) {
            if (!deferClose)
                closeFd(stdin_pipe_[1]);
            markStdinClosed();
        }
        return CGI_IO_PROGRESS;
    }
    if (!deferClose)
        closeFd(stdin_pipe_[1]);
    markStdinClosed();
    fail();
    return CGI_IO_ERROR;
}

// Read one chunk and retain binary bytes; EOF is distinct from child reaping.
CgiProcess::IoResult CgiProcess::readOutput() {
    return readOutput(false);
}

// Transfer one chunk with optional deferred descriptor closure.
CgiProcess::IoResult CgiProcess::readOutput(bool deferClose) {
    return readOutput(deferClose, MAX_OUTPUT_BYTES);
}

// Enforce both budgets before appending a chunk.
CgiProcess::IoResult CgiProcess::readOutput(bool deferClose, std::size_t globalRemaining) {
    if (state_ != CGI_RUNNING || stdout_eof_ || stdout_pipe_[0] < 0)
        return CGI_IO_INVALID;

    char buffer[8192];
    const ssize_t n = read(stdout_pipe_[0], buffer, sizeof(buffer));
    if (n > 0) {
        if (static_cast<std::size_t>(n) > MAX_OUTPUT_BYTES - output_buffer_.size()) {
            fail();
            return CGI_IO_TOO_LARGE;
        }
        if (static_cast<std::size_t>(n) > globalRemaining) {
            fail();
            return CGI_IO_GLOBAL_LIMIT;
        }
        appendOutput(buffer, static_cast<std::size_t>(n));
        return CGI_IO_PROGRESS;
    }
    if (n == 0) {
        if (!deferClose)
            closeFd(stdout_pipe_[0]);
        markStdoutEof();
        return CGI_IO_CLOSED;
    }
    fail();
    return CGI_IO_ERROR;
}

// Read a possibly fragmented native int; EOF without data indicates exec success.
// An incomplete integer at EOF is a protocol error, not a successful exec.
CgiProcess::IoResult CgiProcess::readExecError() {
    return readExecError(false);
}

// Transfer one chunk with optional deferred descriptor closure.
CgiProcess::IoResult CgiProcess::readExecError(bool deferClose) {
    if (state_ != CGI_RUNNING || exec_error_eof_ || exec_error_received_ ||
        exec_error_pipe_[0] < 0)
        return CGI_IO_INVALID;

    const ssize_t n = read(exec_error_pipe_[0],
        exec_error_bytes_ + exec_error_offset_, sizeof(int) - exec_error_offset_);
    if (n > 0) {
        exec_error_offset_ = exec_error_offset_ + static_cast<std::size_t>(n);
        if (exec_error_offset_ == sizeof(int)) {
            std::memcpy(&exec_error_code_, exec_error_bytes_, sizeof(int));
            exec_error_received_ = true;
            if (!deferClose)
                closeFd(exec_error_pipe_[0]);
            fail();
        }
        return CGI_IO_PROGRESS;
    }
    if (n == 0) {
        exec_error_eof_ = true;
        if (!deferClose)
            closeFd(exec_error_pipe_[0]);
        if (exec_error_offset_ != 0) {
            fail();
            return CGI_IO_ERROR;
        }
        tryFinish();
        return CGI_IO_CLOSED;
    }
    fail();
    return CGI_IO_ERROR;
}

// Check only this object's direct child; WNOHANG keeps the event loop responsive.
// EINTR means that no child status was collected during this call.
CgiProcess::WaitResult CgiProcess::pollChild() {
    if (pid_ <= 0 || child_exited_)
        return CGI_WAIT_INVALID;

    int status = 0;
    const pid_t result = waitpid(pid_, &status, WNOHANG);
    if (result == 0)
        return CGI_WAIT_RUNNING;
    if (result == pid_) {
        markChildExited(status);
        return CGI_WAIT_REAPED;
    }
    if (result == -1 && errno == EINTR)
        return CGI_WAIT_INTERRUPTED;
    // An unexpected waitpid error requires the manager to reconcile ownership.
    // In particular, ECHILD is not proof that this object reaped the child.
    return CGI_WAIT_ERROR;
}

// Send a signal only to our positive PID while the child is not reaped.
// ESRCH means the process no longer exists; it still needs waitpid reconciliation.
CgiProcess::SignalResult CgiProcess::signalChild(int signalNumber) {
    if (pid_ <= 0 || child_exited_)
        return CGI_SIGNAL_INVALID;
    if (kill(pid_, signalNumber) == 0)
        return CGI_SIGNAL_SENT;
    if (errno == ESRCH)
        return CGI_SIGNAL_ALREADY_EXITED;
    return CGI_SIGNAL_ERROR;
}

// Ask a running or terminal-but-unreaped child to exit gracefully.
CgiProcess::SignalResult CgiProcess::requestTermination() {
    return signalChild(SIGTERM);
}

// Escalate termination for a child that has not yet been reaped.
CgiProcess::SignalResult CgiProcess::forceKill() {
    return signalChild(SIGKILL);
}

// Start cleanup without blocking or losing the original terminal failure reason.
// Close pipes first so no new I/O can occur while the child is being stopped.
CgiProcess::CleanupResult CgiProcess::beginCleanup() {
    if (!cleanup_started_) {
        cleanup_started_ = true;
        closePipes();
        stdin_closed_ = true;
        if (state_ == CGI_RUNNING)
            fail();
    }
    if (pid_ <= 0 || child_exited_)
        return CGI_CLEANUP_DONE;

    // Reap first: do not signal a child that has already terminated.
    const WaitResult result = pollChild();
    if (result == CGI_WAIT_REAPED)
        return CGI_CLEANUP_DONE;
    if (result == CGI_WAIT_ERROR)
        return CGI_CLEANUP_ERROR;
    if (result == CGI_WAIT_INTERRUPTED)
        return CGI_CLEANUP_INTERRUPTED;

    const SignalResult signalled = requestTermination();
    if (signalled == CGI_SIGNAL_ERROR || signalled == CGI_SIGNAL_INVALID)
        return CGI_CLEANUP_ERROR;
    return CGI_CLEANUP_PENDING;
}

// Recheck cleanup progress; never block or signal again automatically.
// SIGKILL escalation and retry timing are owned by CgiManager.
CgiProcess::CleanupResult CgiProcess::continueCleanup() {
    if (!cleanup_started_)
        return CGI_CLEANUP_ERROR;
    if (pid_ <= 0 || child_exited_)
        return CGI_CLEANUP_DONE;

    const WaitResult result = pollChild();
    if (result == CGI_WAIT_REAPED)
        return CGI_CLEANUP_DONE;
    if (result == CGI_WAIT_RUNNING)
        return CGI_CLEANUP_PENDING;
    if (result == CGI_WAIT_INTERRUPTED)
        return CGI_CLEANUP_INTERRUPTED;
    return CGI_CLEANUP_ERROR;
}

// Close a matching endpoint only after logical completion or terminal state.
bool CgiProcess::closeCompletedEndpoint(int fd) {
    if (fd < 0)
        return false;
    if (fd == stdin_pipe_[1] && (stdin_closed_ || terminal())) {
        closeFd(stdin_pipe_[1]);
        return true;
    }
    if (fd == stdout_pipe_[0] && (stdout_eof_ || terminal())) {
        closeFd(stdout_pipe_[0]);
        return true;
    }
    if (fd == exec_error_pipe_[0] &&
        (exec_error_eof_ || exec_error_received_ || terminal())) {
        closeFd(exec_error_pipe_[0]);
        return true;
    }
    return false;
}

// Swap only after the execution is terminal and cleanup has started.
bool CgiProcess::takeOutput(std::string& destination) {
    if (!terminal() || !cleanup_started_ || !destination.empty())
        return false;
    output_buffer_.swap(destination);
    return true;
}

// Release failed output storage, not merely its logical length.
void CgiProcess::discardFailedOutput() {
    if (state_ == CGI_FAILED || state_ == CGI_TIMED_OUT || state_ == CGI_CANCELLED) {
        std::string empty;
        output_buffer_.swap(empty);
    }
}

CgiProcess::State CgiProcess::state() const { 
    return state_; 
}

pid_t CgiProcess::pid() const { 
    return pid_; 
}

unsigned long CgiProcess::connectionId() const { 
    return connection_id_; 
}

bool CgiProcess::stdinClosed() const { 
    return stdin_closed_; 
}

bool CgiProcess::stdoutEof() const { 
    return stdout_eof_; 
}

bool CgiProcess::childExited() const { 
    return child_exited_; 
}

int CgiProcess::childExitStatus() const { 
    return child_exit_status_; 
}

const std::string& CgiProcess::inputBuffer() const { 
    return input_buffer_; 
}

const std::string& CgiProcess::outputBuffer() const { 
    return output_buffer_; 
}

std::size_t CgiProcess::inputOffset() const { 
    return input_offset_; 
}

int CgiProcess::parentStdinFd() const { 
    return stdin_pipe_[1]; 
}

int CgiProcess::parentStdoutFd() const { 
    return stdout_pipe_[0]; 
}

int CgiProcess::parentExecErrorFd() const { 
    return exec_error_pipe_[0]; 
}

int CgiProcess::childStdinFd() const { 
    return stdin_pipe_[0]; 
}

int CgiProcess::childStdoutFd() const { 
    return stdout_pipe_[1]; 
}

int CgiProcess::childExecErrorFd() const { 
    return exec_error_pipe_[1]; 
}

bool CgiProcess::execErrorReceived() const { 
    return exec_error_received_; 
}

int CgiProcess::execErrorCode() const { 
    return exec_error_code_; 
}

bool CgiProcess::execErrorEof() const { 
    return exec_error_eof_; 
}

bool CgiProcess::cleanupStarted() const { 
    return cleanup_started_; 
}
