# Logging

Design of the logging module for the Webserver. This is the *specification*;
implement it in `src/logger.cpp` + `inc/logger.hpp`.

## Purpose

The Webserver is a single process with an event loop; logs are the only way
to diagnose what happened with a given connection (a stuck client, a failing
CGI, a malformed request). Every module logs through the `Logger`.

## Levels

| Level   | Use                                                  |
| ------- | ---------------------------------------------------- |
| `DEBUG` | fine-grained trace: parse states, bytes read/written |
| `INFO`  | lifecycle: listening, connections, served requests   |
| `WARN`  | unusual but non-fatal: timeouts, connection resets   |
| `ERROR` | syscall / CGI failures, mapped to HTTP 5xx           |

Threshold is configurable at startup. Default: `DEBUG` in dev builds,
`INFO` otherwise. Lines below the threshold are not emitted.

## Output

- **stderr** always.
- **Optional file**: if a path is given (e.g. `./logs/webserv.log`), the same
  lines are appended to it. Use `std::ofstream` (C++98). File I/O errors are
  silently ignored — logging must never break the server.

## Line format

```
[YYYY-MM-DD HH:MM:SS] [LEVEL] file:line - message
```

Timestamp via `gettimeofday()`/`time()` (no `std::chrono` in C++98),
converted to UTC/GMT with `gmtime`.

## Access log

A per-request line (always `INFO`), nginx-combined style:

```
127.0.0.1 - "GET /index.html HTTP/1.1" 200 1234
```

Emitted once per request when the response is sent: remote address, request
line, status code, body bytes. This is the primary tool for `curl`-driven
testing.

## API

Simple `Logger` class + macros (C++98 — no lambdas, no exceptions in the
happy path):

```cpp
// inc/logger.hpp
class Logger {
public:
    enum Level { DEBUG, INFO, WARN, ERROR };
    static Logger &instance();
    void setLevel(Level level);
    void setFile(const std::string &path);
    void log(Level level, const char *file, int line, const std::string &msg);
    void accessLog(const std::string &remote, const std::string &request_line,
                   int status, std::size_t bytes);
private:
    Logger();
    Level level_;
    std::ofstream file_;
};

#define LOG_DEBUG(m) Logger::instance().log(Logger::DEBUG, __FILE__, __LINE__, m)
#define LOG_INFO(m)  Logger::instance().log(Logger::INFO,  __FILE__, __LINE__, m)
#define LOG_WARN(m)  Logger::instance().log(Logger::WARN,  __FILE__, __LINE__, m)
#define LOG_ERROR(m) Logger::instance().log(Logger::ERROR, __FILE__, __LINE__, m)
#define LOG_ACCESS(r, q, s, b) \
    Logger::instance().accessLog((r), (q), (s), (b))
```

`LOG_ACCESS` lives in the **same `Logger` class** as a dedicated method, not
a separate class: it shares the output (stderr + file) and timestamp
formatting, but is a distinct channel. Unlike the diagnostic macros it is
**always emitted** (not filtered by level threshold) and carries no
`file:line`, because it is a record of a request, not a code trace. It
builds the access line described above (arguments: remote address, request
line, status, body bytes):

```
[YYYY-MM-DD HH:MM:SS] [ACCESS] 127.0.0.1 - "GET /index.html HTTP/1.1" 200 1234
```

## Where to log

| Event                 | Level   | Example message                                   |
| --------------------- | ------- | ------------------------------------------------- |
| Server start          | `INFO`  | `listening on 0.0.0.0:8080`                       |
| Server shutdown       | `INFO`  | `signal received, shutting down`                  |
| Connection accepted   | `DEBUG` | `accepted fd 7`                                   |
| Connection closed     | `DEBUG` | `closing fd 7`                                    |
| Connection timeout    | `WARN`  | `idle timeout on fd 7`                            |
| Request served        | access  | `127.0.0.1 - "GET / HTTP/1.1" 200 120`            |
| CGI start / exit      | `DEBUG` | `cgi for /cgi-bin/script.py pid 1234`             |
| CGI failure           | `ERROR` | `cgi failed for /cgi-bin/broken.py`               |
| Syscall failure       | `ERROR` | `accept failed on fd 5`                           |

## Constraints

- **No `errno` after `read`/`write`** (subject rule). Do not log
  `strerror(errno)` following socket operations; use your own messages.
- **Single-threaded**: no locks, no atomics needed.
- **No sensitive data**: never log request bodies, headers or config values
  that may hold secrets.
- Logging must be allocation-light on the hot path; formatting with
  `ostringstream` is acceptable.
- Production code logs through the macros only — no bare `printf`/`cerr`
  (see [CONVENTIONS.md](CONVENTIONS.md#logging--assertions)).
