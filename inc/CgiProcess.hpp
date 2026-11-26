#ifndef CGI_PROCESS_HPP
#define CGI_PROCESS_HPP

#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <exception>
#include <signal.h>
#include <cstring>
#include <cstddef>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>

// Track one CGI execution independently from the server event loop.
// Own pipe endpoints and supervise one direct child without blocking.
class CgiProcess {
public:
    enum State {
        CGI_CREATED,
        CGI_RUNNING,
        CGI_FINISHED,
        CGI_FAILED,
        CGI_TIMED_OUT,
        CGI_CANCELLED
    };

    enum OutputResult {
        CGI_OUTPUT_OK,
        CGI_OUTPUT_TOO_LARGE,
        CGI_OUTPUT_INVALID_STATE
    };

    enum IoResult {
        CGI_IO_PROGRESS,
        CGI_IO_AGAIN,
        CGI_IO_CLOSED,
        CGI_IO_ERROR,
        CGI_IO_TOO_LARGE,
        CGI_IO_GLOBAL_LIMIT,
        CGI_IO_INVALID
    };

    static const std::size_t MAX_OUTPUT_BYTES = 10u * 1024u * 1024u;

    CgiProcess();
    ~CgiProcess();

    enum LaunchResult {
        CGI_LAUNCH_STARTED,
        CGI_LAUNCH_INVALID,
        CGI_LAUNCH_PIPE_ERROR,
        CGI_LAUNCH_FORK_ERROR
    };

    // Fork a CGI child using previously built execve arrays.
    // The caller owns launchData and must keep it valid through this call.
    LaunchResult launch(const std::string& executable, char* const* argv,
        char* const* envp);

    // Allocate three pipes and make only the future parent ends nonblocking.
    // On failure all descriptors are closed and the object stays CREATED.
    bool createPipes();

    // Close ends that belong to the other process after fork succeeds.
    void closeChildEndsInParent();
    void closeParentEndsInChild();

    // Close every descriptor still owned by this object (idempotent).
    void closePipes();

    int parentStdinFd() const;
    int parentStdoutFd() const;
    int parentExecErrorFd() const;
    int childStdinFd() const;
    int childStdoutFd() const;
    int childExecErrorFd() const;

    // Transfer one nonblocking chunk; the caller schedules subsequent readiness.
    IoResult writeInput();
    IoResult readOutput();
    IoResult readExecError();
    // Managed I/O leaves logically completed endpoints open until epoll DEL.
    IoResult writeInput(bool deferClose);
    IoResult readOutput(bool deferClose);
    IoResult readOutput(bool deferClose, std::size_t globalRemaining);
    // Transfer terminal stdout without copying; caller owns the destination.
    bool takeOutput(std::string& destination);
    // Release stdout that cannot be delivered as a successful result.
    void discardFailedOutput();
    IoResult readExecError(bool deferClose);

    // Close only a matching completed endpoint, or an endpoint of a terminal CGI.
    // Caller must retire its event registration first.
    bool closeCompletedEndpoint(int fd);
    bool execErrorReceived() const;
    int execErrorCode() const;
    bool execErrorEof() const;

    enum WaitResult {
        CGI_WAIT_RUNNING,
        CGI_WAIT_REAPED,
        CGI_WAIT_INTERRUPTED,
        CGI_WAIT_ERROR,
        CGI_WAIT_INVALID
    };

    enum SignalResult {
        CGI_SIGNAL_SENT,
        CGI_SIGNAL_ALREADY_EXITED,
        CGI_SIGNAL_ERROR,
        CGI_SIGNAL_INVALID
    };

    // Poll the direct child once without blocking; preserve terminal states.
    WaitResult pollChild();

    // Signal only the known, not-yet-reaped direct child.
    // The manager chooses when to request termination or escalate to SIGKILL.
    SignalResult requestTermination();
    SignalResult forceKill();

    enum CleanupResult {
        CGI_CLEANUP_DONE,
        CGI_CLEANUP_PENDING,
        CGI_CLEANUP_INTERRUPTED,
        CGI_CLEANUP_ERROR
    };

    // Start nonblocking cleanup: close all pipes and request SIGTERM if needed.
    // The manager retains ownership and schedules follow-up cleanup calls.
    CleanupResult beginCleanup();

    // Poll for reaping without blocking; manager may call forceKill after grace.
    CleanupResult continueCleanup();
    bool cleanupStarted() const;

    // Prepare a process-owned copy of the request body before any fork.
    bool prepare(unsigned long connectionId, const std::string& requestBody);

    // Record the child PID only after a successful fork in a later increment.
    bool markRunning(pid_t childPid);

    // Record independent lifecycle signals; neither alone finishes the CGI.
    void markChildExited(int waitStatus);
    void markStdoutEof();
    void markStdinClosed();

    // Append binary-safe stdout without exceeding the per-CGI budget.
    OutputResult appendOutput(const char* bytes, std::size_t length);

    // Terminal failures cannot be overwritten by late child or pipe events.
    void fail();
    void timeOut();
    void cancel();

    State state() const;
    pid_t pid() const;
    unsigned long connectionId() const;
    bool stdinClosed() const;
    bool stdoutEof() const;
    bool childExited() const;
    int childExitStatus() const;
    const std::string& inputBuffer() const;
    const std::string& outputBuffer() const;
    std::size_t inputOffset() const;

    // Record bytes already written to CGI stdin; never exceed body length.
    bool advanceInput(std::size_t count);

private:
    int stdin_pipe_[2];
    int stdout_pipe_[2];
    int exec_error_pipe_[2];
    State state_;
    pid_t pid_;
    unsigned long connection_id_;
    bool stdin_closed_;
    bool stdout_eof_;
    bool child_exited_;
    int child_exit_status_;
    std::string input_buffer_;
    std::size_t input_offset_;
    std::string output_buffer_;
    unsigned char exec_error_bytes_[sizeof(int)];
    std::size_t exec_error_offset_;
    bool exec_error_received_;
    bool exec_error_eof_;
    int exec_error_code_;
    bool cleanup_started_;

    CgiProcess(const CgiProcess& other);
    CgiProcess& operator=(const CgiProcess& other);

    static void closeFd(int& fd);
    static bool setNonBlocking(int fd);
    static bool setCloseOnExec(int fd);
    bool terminal() const;
    void tryFinish();
    SignalResult signalChild(int signalNumber);
};

#endif
