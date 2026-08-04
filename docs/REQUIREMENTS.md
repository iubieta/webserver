# Webserv — Subject Requirements Checklist

Source: Webserv subject, version 24.0 (42 curriculum).
Use this as a progress tracker and as a defense script.

---

## I. General Rules

- [ ] Program never crashes under any circumstance (including running out
      of memory) and never terminates unexpectedly -> otherwise grade = 0
- [ ] `Makefile` provided, compiles all sources, **no unnecessary
      relinking**
- [ ] Makefile rules: `$(NAME)`, `all`, `clean`, `fclean`, `re`
- [ ] Compiles with `c++` and the flags `-Wall -Wextra -Werror`
- [ ] Complies with **C++98**, still compiles when adding `-std=c++98`
- [ ] Prefers C++ headers and features over C ones (`<cstring>` over
      `<string.h>`, etc.)
- [ ] No external libraries, no Boost

---

## II. Mandatory — Program and Build

- [ ] Executable named `webserv`
- [ ] Files submitted: `Makefile`, `*.{h,hpp}`, `*.cpp`, `*.tpp`, `*.ipp`,
      configuration files
- [ ] Runs as `./webserv [configuration file]`
- [ ] Only allowed external functions used:
      `execve`, `pipe`, `strerror`, `gai_strerror`, `errno`, `dup`,
      `dup2`, `fork`, `socketpair`, `htons`, `htonl`, `ntohs`, `ntohl`,
      `select`, `poll`, `epoll_create`, `epoll_ctl`, `epoll_wait`,
      `kqueue`, `kevent`, `socket`, `accept`, `listen`, `send`, `recv`,
      `chdir`, `bind`, `connect`, `getaddrinfo`, `freeaddrinfo`,
      `setsockopt`, `getsockname`, `getprotobyname`, `fcntl`, `close`,
      `read`, `write`, `waitpid`, `kill`, `signal`, `access`, `stat`,
      `open`, `opendir`, `readdir`, `closedir`
- [ ] No libft

---

## III. Mandatory — I/O Model (the part that gets you a 0)

- [ ] Server is **non-blocking at all times**
- [ ] **Exactly one** `poll()` (or `select` / `epoll` / `kqueue`) for
      **all** client-server I/O, listening sockets included
- [ ] That single call monitors **reading and writing simultaneously**
- [ ] **Never** `read` / `recv` / `write` / `send` on a socket or pipe
      without prior readiness notification from poll
- [ ] **Never** check `errno` after a read or write to adjust behaviour
Regular disk files are exempt from poll (direct read/write allowed)
- [ ] Client disconnections handled properly
- [ ] No request ever hangs indefinitely (timeouts implemented)
- [ ] `fork()` used **only** for CGI
- [ ] No `execve` of another web server
      and `FD_CLOEXEC`; any other flag is forbidden

---

## IV. Mandatory — HTTP Behaviour

- [ ] Config file taken from the command line **or** from a default path
- [ ] Listens on **multiple ports**, serving different content per port
- [ ] Compatible with a standard web browser of your choice
- [ ] Serves a **fully static website**
- [ ] `GET`, `POST` and `DELETE` implemented at minimum
- [ ] **File upload** from clients works
- [ ] HTTP status codes are **accurate**
- [ ] **Default error pages** built in when none are configured
- [ ] Behaviour and headers comparable to NGINX (mind HTTP version
      differences)
- [ ] Stress-tested and stays available at all times (resilience)
- [ ] Tested with more than one program (CGI); tests written in another
      language (Python, Go, C, C++, etc.)
- [ ] Tested with `telnet` and against NGINX
- [ ] Virtual hosts: **out of scope**, optional

---

## V. Mandatory — Configuration File

The config file must allow you to:

- [ ] Define all `interface:port` pairs to listen on (multiple websites)
- [ ] Set up default error pages
- [ ] Set the maximum allowed size for client request bodies
- [ ] Define per-route rules (no regex required):
  - [ ] List of accepted HTTP methods for the route
  - [ ] HTTP redirection
  - [ ] Root directory mapping: `/kapouet` rooted to `/tmp/www` means
        `/kapouet/pouic/toto/pouet` resolves to
        `/tmp/www/pouic/toto/pouet`
  - [ ] Enable or disable directory listing (autoindex)
  - [ ] Default file to serve when the request targets a directory
  - [ ] Enable uploads and provide the upload storage location
  - [ ] CGI execution based on file extension (for example `.php`)
- [ ] Config files and default files provided to demo **every** feature
      during the evaluation

### CGI specifics

- [ ] Correct environment variables set for server-CGI communication
- [ ] Full request and client-provided arguments available to the CGI
- [ ] Chunked requests **un-chunked by the server**; the CGI receives EOF
      as the end of the body
- [ ] CGI output: if no `Content-Length` is returned, EOF marks the end
      of the returned data
- [ ] CGI executed in the correct working directory so relative paths
      resolve
- [ ] At least one CGI supported (php-CGI, Python, etc.)

---

## VI. README.md (root of the repo, written in English)

- [ ] First line, italicized: *This project has been created as part of
      the 42 curriculum by \<login1\>[, \<login2\>[, \<login3\>[...]]].*
- [ ] "Description" section: goal and brief overview
- [ ] "Instructions" section: compilation, installation, execution
- [ ] "Resources" section: classic references, plus a description of how
      AI was used, for which tasks and which parts of the project
- [ ] Extra sections as needed (usage examples, feature list, technical
      choices)

---

## VII. Bonus (only graded if the mandatory part is flawless)

- [ ] Cookies and session management, with simple working examples
- [ ] Multiple CGI types supported

---

## VIII. Submission and Defense

- [ ] Everything pushed to the Git repository; filenames double-checked
- [ ] Ready to perform a small live modification: behaviour change, a few
      lines to write or rewrite, or an easy-to-add feature
- [ ] Every team member can explain any part of the code
