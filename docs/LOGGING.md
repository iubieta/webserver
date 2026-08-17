# Logging

The `Logger` class provides a simple log tool for the Webserver (or any other
project). This document describes the **current implementation** in
`src/Logger.cpp` + `inc/Logger.hpp`.

## Overview

`Logger` is an **instanciable** class (not a singleton) with two independent
outputs:

- **File**: optional, opened in **append** mode (`std::ofstream`). Only used
  if a file path is given at construction.
- **Console**: `stderr`, enabled by default.

Each output has its own **level threshold**, so a message can go to one
output, both, or neither.

## Levels

```cpp
enum Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };
```

| Level      | Use                                                  |
| ---------- | ---------------------------------------------------- |
| `DEBUG`    | fine-grained trace: parse states, bytes read/written |
| `INFO`     | lifecycle: listening, connections, served requests   |
| `WARNING`  | unusual but non-fatal: timeouts, connection resets   |
| `ERROR`    | syscall / CGI failures, mapped to HTTP 5xx           |
| `CRITICAL` | fatal conditions                                     |

Filtering is **cumulative**: a message is emitted if `level >= threshold`.
The enum order `DEBUG < INFO < WARNING < ERROR < CRITICAL` defines the
ordering.

## Line format

```
[YYYY-MM-DD HH:MM:SS] [LEVEL] message - file:line
```

Parts:

- `[YYYY-MM-DD HH:MM:SS]` — timestamp via `time()` + `localtime()` +
  `strftime()`, using `TIME_FORMAT` (`%Y-%m-%d %H:%M:%S`). Can be disabled.
- `[LEVEL]` — one of `[DEBUG]`, `[INFO]`, `[WARNING]`, `[ERROR]`,
  `[CRITICAL]`.
- `message` — the logged text.
- `- file:line` — optional source location, only written to the **file** and
  only when `file`/`line` are passed (e.g. `__FILE__`, `__LINE__`).

## Defaults

| Member            | Default                                   |
| ----------------- | ----------------------------------------- |
| `fileLevel_`      | `DEBUG`                                   |
| `console_`        | `true` (console output on)                |
| `consoleLevel_`   | `INFO`                                    |
| `timestamp_`      | `true`                                    |

## API

```cpp
// inc/Logger.hpp
class Logger {
public:
    enum Level { DEBUG, INFO, WARNING, ERROR, CRITICAL };

    Logger();
    Logger(const std::string &filepath);   // open log file in append mode
    ~Logger();

    void setFileLevel(Level level);
    void setConsole(bool enabled);
    void setConsoleLevel(Level level);
    void setTimestamp(bool enabled);

    void debug(const std::string &message, const char *file = NULL, int line = 0) const;
    void info(const std::string &message, const char *file = NULL, int line = 0) const;
    void warning(const std::string &message, const char *file = NULL, int line = 0) const;
    void error(const std::string &message, const char *file = NULL, int line = 0) const;
    void critical(const std::string &message, const char *file = NULL, int line = 0) const;
};
```

### Constructors

- `Logger()` — console only (`stderr`), no file output.
- `Logger(const std::string &filepath)` — same as above plus the file opened
  in append mode. If the file **cannot be opened**, a `WARNING` is logged to
  the console and the logger keeps running in console-only mode. Logging must
  never break the server.

### Setters

- `setFileLevel(level)` — file threshold.
- `setConsole(bool)` — enable/disable console output.
- `setConsoleLevel(level)` — console threshold.
- `setTimestamp(bool)` — enable/disable the timestamp prefix.

File and console levels are **independent**: changing one does not affect the
other.

### Logging methods

One method per level; all follow the same signature. `file`/`line` are
optional and, when given, are appended to the **file** output only. The
console output never carries `file:line`.

## Usage example

```cpp
Logger log("logs/webserv.log");
log.setFileLevel(Logger::DEBUG);
log.setConsoleLevel(Logger::WARNING);   // quiet console, verbose file

log.debug("accepted fd 7", __FILE__, __LINE__);
log.info("request served");
log.warning("idle timeout on fd 7");
log.error("accept failed on fd 5");
log.critical("fatal: out of memory");
```

## Where to log

| Event                 | Level     | Example message                                   |
| --------------------- | --------- | ------------------------------------------------- |
| Server start          | `INFO`    | `listening on 0.0.0.0:8080`                       |
| Server shutdown       | `INFO`    | `signal received, shutting down`                  |
| Connection accepted   | `DEBUG`   | `accepted fd 7`                                   |
| Connection closed     | `DEBUG`   | `closing fd 7`                                    |
| Connection timeout    | `WARNING` | `idle timeout on fd 7`                            |
| CGI start / exit      | `DEBUG`   | `cgi for /cgi-bin/script.py pid 1234`             |
| CGI failure           | `ERROR`   | `cgi failed for /cgi-bin/broken.py`               |
| Syscall failure       | `ERROR`   | `accept failed on fd 5`                           |

## Constraints

- **C++98** only: no lambdas, no exceptions in the happy path.
- **No `errno` after `read`/`write`** (subject rule). Do not log
  `strerror(errno)` following socket operations; use your own messages.
- **Single-threaded**: no locks, no atomics needed.
- **No sensitive data**: never log request bodies, headers or config values
  that may hold secrets.
- **No bare `printf`/`cerr`** in production code: every module logs through
  the `Logger` (see [CONVENTIONS.md](CONVENTIONS.md#logging--assertions)).
- Logging must be allocation-light on the hot path; formatting with
  `ostringstream` is acceptable.

## No implementado: Access log

> **Estado: pendiente.** Lo que sigue es un *diseño esperado* para una tarea
> futura del [ROADMAP](ROADMAP.md) (access log line on response); **no** está
> implementado en el `Logger` actual.

The access log records **one line per served request** (a record of the
request, not a code trace), nginx-combined style:

```
[YYYY-MM-DD HH:MM:SS] [ACCESS] 127.0.0.1 - "GET /index.html HTTP/1.1" 200 1234
```

Expected behaviour / guidelines:

- **Always emitted** once per request when the response is sent — *not*
  filtered by the level threshold (it is a distinct channel).
- Carries **no** `file:line`.
- Fields: timestamp, remote address, request line, status code, body bytes.
- Suggested API — a dedicated method on `Logger` sharing its outputs
  (stderr + file) and timestamp formatting:

  ```cpp
  void accessLog(const std::string &remote, const std::string &request_line,
                 int status, std::size_t bytes);
  ```

- Primary tool for `curl`-driven testing.
