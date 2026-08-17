This project has been created as part of the 42 curriculum by iubieta- and
ypacileo.

# Webserver

HTTP/1.1 web server written in C++98, built as part of the 42 curriculum.
This is a non-blocking server that parses a configuration file and serves
static files and CGI scripts.

## Authors

- iubieta-
- ypacileo

## Repository layout

```
├── config/   # example configuration files
├── docs/     # project documentation (see table of contents below)
├── inc/      # headers (.hpp)
├── res/      # static resources served as error pages / assets
├── src/      # sources (.cpp)
├── tests/    # test suite and fixtures
├── www/      # default web root (static content + cgi-bin)
├── Makefile
└── README.md
```

## Requirements

- C++98 compiler (`c++` with `-std=c++98`)
- GNU `make`
- Linux (poll-based multiplexing)

## Build

```sh
make          # compile the project
make clean    # remove object files
make re       # clean + rebuild
make fclean   # remove objects, binary and dependencies
make test     # build and run the unit tests (./run_tests)
```

## Run

```sh
./webserv [config_file]
```

If no config file is given, a default configuration is used (see
`config/`). See [docs/CHEATSHEET.md](docs/CHEATSHEET.md) for usage examples.

## Documentation

| File                         | Contents                                  |
| ---------------------------- | ----------------------------------------- |
| [docs/ALLOWED-FUNCS.md](docs/ALLOWED-FUNCS.md) | Allowed functions summary        |
| [docs/CHEATSHEET.md](docs/CHEATSHEET.md)      | Build/run/test usage cheatsheet   |
| [docs/CONVENTIONS.md](docs/CONVENTIONS.md)    | Code style and git conventions    |
| [docs/DECISIONS.md](docs/DECISIONS.md)        | Project's arqitecture decisions log    |
| [docs/en.subject.pdf](docs/en.subject.pdf)    | 42 official subject               |
| [docs/GLOSSARY.md](docs/GLOSSARY.md)          | Project concepts and terms definition |
| [docs/HTTP-REFERENCE.md](docs/HTTP-REFERENCE.md) | HTTP/1.1 quick reference       |
| [docs/LOGGING.md](docs/LOGGING.md)            | Logging module design             |
| [docs/QUESTIONS.md](docs/QUESTIONS.md)        | Emerging and solved question log  |
| [docs/REQUIREMENTS.md](docs/REQUIREMENTS.md)  | Subject's requirement checklist   |
| [docs/ROADMAP.md](docs/ROADMAP.md)            | Project milestones and progress   |
| [docs/TESTING.md](docs/TESTING.md)            | How to run and write tests        |

## Resources

### HTTP protocol

- [RFC 7230 — Message Syntax and Routing](https://www.rfc-editor.org/rfc/rfc7230)
  Message framing, header fields, chunked transfer coding and
  connection management. The core reference for our parser.
- [RFC 7231 — Semantics and Content](https://www.rfc-editor.org/rfc/rfc7231)
  Request methods and status code definitions.
- [RFC 2616 — HTTP/1.1 (obsolete)](https://www.rfc-editor.org/rfc/rfc2616)
  Superseded by RFC 7230–7235, but still the version most often cited
  around the subject.
- [RFC 1945 — HTTP/1.0](https://www.rfc-editor.org/rfc/rfc1945)
  The reference point suggested by the subject.
- [MDN — HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)
  Readable companion to the RFCs, especially for headers and status
  codes.
- [MDN — HTTP response status codes](https://developer.mozilla.org/en-US/docs/Web/HTTP/Status)
- [IANA — HTTP status code registry](https://www.iana.org/assignments/http-status-codes/http-status-codes.xhtml)
- [IANA — Media types](https://www.iana.org/assignments/media-types/media-types.xhtml)
  Source for our MIME type table.

### Sockets and I/O multiplexing

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
  Introduction to Berkeley sockets; the sections on server sockets and
  on non-blocking I/O map directly onto this project.
- [poll(2)](https://man7.org/linux/man-pages/man2/poll.2.html)
- [select(2)](https://man7.org/linux/man-pages/man2/select.2.html)
- [epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [kqueue(2)](https://man.freebsd.org/cgi/man.cgi?kqueue)
- [socket(2)](https://man7.org/linux/man-pages/man2/socket.2.html)
- [listen(2)](https://man7.org/linux/man-pages/man2/listen.2.html)
- [accept(2)](https://man7.org/linux/man-pages/man2/accept.2.html)
- [setsockopt(2)](https://man7.org/linux/man-pages/man2/setsockopt.2.html)
- [fcntl(2)](https://man7.org/linux/man-pages/man2/fcntl.2.html)
- [signal(7)](https://man7.org/linux/man-pages/man7/signal.7.html)
  Relevant for `SIGPIPE` handling on closed sockets.
- [tcp(7)](https://man7.org/linux/man-pages/man7/tcp.7.html)

### CGI

- [RFC 3875 — The Common Gateway Interface (CGI) 1.1](https://www.rfc-editor.org/rfc/rfc3875)
  Meta-variables, request body handling and response parsing.
- [execve(2)](https://man7.org/linux/man-pages/man2/execve.2.html)
- [pipe(2)](https://man7.org/linux/man-pages/man2/pipe.2.html)
- [waitpid(2)](https://man7.org/linux/man-pages/man2/wait.2.html)

### Configuration file design

- [NGINX documentation index](https://nginx.org/en/docs/)
- [Module ngx_http_core_module](https://nginx.org/en/docs/http/ngx_http_core_module.html)
  Reference for the `server`, `location`, `root`, `index` and
  `client_max_body_size` directives our configuration imitates.
- [Module ngx_http_autoindex_module](https://nginx.org/en/docs/http/ngx_http_autoindex_module.html)
- [NGINX beginner's guide](https://nginx.org/en/docs/beginners_guide.html)

### C++ and build

- [cppreference — C++ language reference](https://en.cppreference.com/w/cpp)
- [ISO C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
  Used selectively; many entries assume C++11 or later.
- [GNU make manual](https://www.gnu.org/software/make/manual/make.html)

### Testing and inspection

- [curl](https://curl.se/docs/manpage.html)
- [Valgrind user manual](https://valgrind.org/docs/manual/manual.html)
- [siege](https://www.joedog.org/siege-home/)
- [Apache HTTP server benchmarking tool (ab)](https://httpd.apache.org/docs/current/programs/ab.html)
- [Python http.client](https://docs.python.org/3/library/http.client.html)
  Used to write our own protocol-level tests.

