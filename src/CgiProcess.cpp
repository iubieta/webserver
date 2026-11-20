#include "../inc/CgiProcess.hpp"

// Initialize all state without allocating OS resources.
CgiProcess::CgiProcess()
    : state_(CGI_CREATED), pid_(-1), connection_id_(0),
      stdin_closed_(false), stdout_eof_(false), child_exited_(false),
      child_exit_status_(0), input_offset_(0) {}

// Increment 3A has no owned file descriptors or child to release.
CgiProcess::~CgiProcess() {}

// Replace the initial body only while the process has not started.
bool CgiProcess::prepare(unsigned long connectionId, const std::string& requestBody) {
    if (state_ != CGI_CREATED)
        return false;
    input_buffer_ = requestBody;
    connection_id_ = connectionId;
    input_offset_ = 0;
    return true;
}

// Mark a successfully forked child; PID zero is not a valid child PID.
bool CgiProcess::markRunning(pid_t childPid) {
    if (state_ != CGI_CREATED || childPid <= 0)
        return false;
    pid_ = childPid;
    state_ = CGI_RUNNING;
    return true;
}

// A reaped child is not sufficient to finish until stdout also reaches EOF.
void CgiProcess::markChildExited(int waitStatus) {
    if (state_ != CGI_RUNNING || child_exited_)
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
    if (!terminal()) state_ = CGI_FAILED;
}

// Record a terminal timeout without overwriting an earlier terminal state.
void CgiProcess::timeOut() {
    if (!terminal()) state_ = CGI_TIMED_OUT;
}

// Record a disconnected client without overwriting an earlier terminal state.
void CgiProcess::cancel() {
    if (!terminal()) state_ = CGI_CANCELLED;
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

// Advance only by the number of bytes confirmed written to the stdin pipe.
bool CgiProcess::advanceInput(std::size_t count) {
    if (state_ != CGI_RUNNING || stdin_closed_ || count > input_buffer_.size() - input_offset_)
        return false;
    input_offset_ += count;
    return true;
}

bool CgiProcess::terminal() const {
    return state_ == CGI_FINISHED || state_ == CGI_FAILED ||
           state_ == CGI_TIMED_OUT || state_ == CGI_CANCELLED;
}

// Only both child reaping and stdout EOF can complete a running process.
void CgiProcess::tryFinish() {
    if (state_ == CGI_RUNNING && child_exited_ && stdout_eof_)
        state_ = CGI_FINISHED;
}
