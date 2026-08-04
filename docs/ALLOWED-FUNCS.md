# Allowed functions — Webserv (42, subject v24.0)

Quick reference for the external functions authorized by the subject.
For each one: header, prototype, what the man page says, and what it is
actually for in the project. Remember: everything must be implemented
in C++98 and, when a C++ flavour of a utility exists, prefer it
(`<cstring>` over `<string.h>`, and so on).

---

## 0. Subject rules that constrain how you use them

- Any I/O that can wait for data (sockets, pipes/FIFOs) must be
  non-blocking and driven by a SINGLE `poll()` (or equivalent).
  Reading or writing those fds without prior readiness = grade 0.
- Regular disk files are exempt: `read()`/`write()` on them do not
  require readiness notifications.
- Checking `errno` to adjust behaviour AFTER a `read`/`write` is
  strictly forbidden. (You may still use it for `bind`, `socket`,
  `open`... when printing start-up error messages.)
- `fork()` is only allowed for CGI. You cannot `execve` another web
  server.
- `fcntl()` is allowed only with `F_SETFL`, `O_NONBLOCK` and
  `FD_CLOEXEC`. Any other flag is forbidden (note: `F_GETFL` is not on
  the list, so the classic get-then-set pattern is out).

---

## 1. Socket creation and management

### socket
`int socket(int domain, int type, int protocol);` — `<sys/socket.h>`
Creates a communication endpoint and returns a file descriptor.
In webserv: `socket(AF_INET, SOCK_STREAM, 0)` for every interface:port
pair declared in the configuration file.

### setsockopt
```
int setsockopt(int fd, int level, int optname,
               const void *optval, socklen_t optlen);
```
`<sys/socket.h>` — Sets options on a socket.
In webserv: `SO_REUSEADDR` so you can restart the server without
waiting for the port's TIME_WAIT to expire. Must be called BEFORE
`bind`.

### bind
```
int bind(int fd, const struct sockaddr *addr, socklen_t len);
```
`<sys/socket.h>` — Assigns a local address (IP + port) to the socket.
In webserv: this is where the `listen host:port` directive becomes
real.

### listen
`int listen(int fd, int backlog);` — `<sys/socket.h>`
Marks the socket as passive: it will accept incoming connections and
queue up to `backlog` pending ones.

### accept
```
int accept(int fd, struct sockaddr *addr, socklen_t *len);
```
`<sys/socket.h>` — Removes the first connection from the queue and
returns a NEW fd for that client. The listening socket stays alive.
In webserv: called when `poll` reports POLLIN on a listening fd. The
returned fd does NOT inherit O_NONBLOCK: set it yourself.

### connect
```
int connect(int fd, const struct sockaddr *addr, socklen_t len);
```
`<sys/socket.h>` — Initiates a connection towards a server.
In webserv: client side only; you will need it if you write your own
testers in C/C++.

### send / recv
```
ssize_t send(int fd, const void *buf, size_t len, int flags);
ssize_t recv(int fd, void *buf, size_t len, int flags);
```
`<sys/socket.h>` — Like `write`/`read` but socket-specific and with
flags. `recv` returning 0 means the peer closed the connection (EOF).
In webserv: `MSG_NOSIGNAL` (Linux) avoids SIGPIPE; on macOS you use
`SO_NOSIGPIPE` or ignore the signal. Never assume a whole buffer goes
out in one call: keep an offset and wait for the next POLLOUT.

### getsockname
```
int getsockname(int fd, struct sockaddr *addr, socklen_t *len);
```
Returns the local address the socket is bound to.
In webserv: useful if you bind on port 0, or to know which interface a
request came in through.

### getaddrinfo / freeaddrinfo
```
int getaddrinfo(const char *node, const char *service,
                const struct addrinfo *hints,
                struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
```
`<netdb.h>` — Translates a host name plus service into a linked list of
`addrinfo` structures ready for `socket`/`bind`/`connect`.
In webserv: the modern alternative to filling a `sockaddr_in` by hand;
it lets you resolve "localhost" or names from the config.
`freeaddrinfo` releases the list (otherwise you leak memory).

### getprotobyname
```
struct protoent *getprotobyname(const char *name);
```
`<netdb.h>` — Looks up /etc/protocols and returns the protocol number.
`getprotobyname("tcp")->p_proto` instead of a magic 0.

---

## 2. Byte order conversion (endianness)

```
uint16_t htons(uint16_t x);   uint32_t htonl(uint32_t x);
uint16_t ntohs(uint16_t x);   uint32_t ntohl(uint32_t x);
```
`<arpa/inet.h>` — Convert between host byte order and network byte
order (big-endian). `h`=host, `n`=network, `s`=short (16b), `l`=long
(32b).
In webserv: `addr.sin_port = htons(8080);`
`addr.sin_addr.s_addr = htonl(INADDR_ANY);`
Key concept: the port always travels big-endian while your x86 is
little-endian; skip this and the port comes out byte-swapped.

---

## 3. I/O multiplexing (pick ONE)

### select
```
int select(int nfds, fd_set *r, fd_set *w, fd_set *e,
           struct timeval *timeout);
```
`<sys/select.h>` — Waits until one of the fds in the sets is ready.
Allowed macros: `FD_ZERO`, `FD_SET`, `FD_CLR`, `FD_ISSET`.
Limitations: capped at `FD_SETSIZE` (1024) and it modifies the sets,
so you must rebuild them on every iteration.

### poll
```
int poll(struct pollfd *fds, nfds_t nfds, int timeout);
```
`<poll.h>` — Same idea with an array of `struct pollfd {fd, events,
revents}`. No 1024 limit and it does not destroy `events`.
In webserv: request `POLLIN|POLLOUT` depending on each client's state
and read `revents` (watch for `POLLHUP`, `POLLERR`, `POLLNVAL` too).
The `timeout` is in ms: use it to drive client and CGI timeouts.

### epoll (Linux)
```
int epoll_create(int size);
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *ev);
int epoll_wait(int epfd, struct epoll_event *evs, int max, int to);
```
`<sys/epoll.h>` — Interest is registered once (`EPOLL_CTL_ADD`, `MOD`,
`DEL`) and the kernel returns only the ready fds: O(1) instead of
O(n). Level-triggered (default, safer) and edge-triggered modes.

### kqueue (macOS/BSD)
```
int kqueue(void);
int kevent(int kq, const struct kevent *changes, int nch,
           struct kevent *events, int nev,
           const struct timespec *timeout);
```
`<sys/event.h>` — The BSD equivalent of epoll. Use `EV_SET` to fill in
the filters (`EVFILT_READ`, `EVFILT_WRITE`).

---

## 4. Processes and CGI

### fork
`pid_t fork(void);` — `<unistd.h>`
Duplicates the process. Returns 0 in the child and the child's PID in
the parent. Allowed only to launch the CGI.

### execve
```
int execve(const char *path, char *const argv[],
           char *const envp[]);
```
`<unistd.h>` — Replaces the current process image with another
program. It only returns if it fails.
In webserv: in the child, launch the interpreter (`/usr/bin/python3`,
`php-cgi`). `envp` is where the CGI variables go: `REQUEST_METHOD`,
`PATH_INFO`, `QUERY_STRING`, `CONTENT_LENGTH`, `CONTENT_TYPE`,
`SCRIPT_NAME`, `SERVER_PROTOCOL`, `HTTP_*`...

### pipe
`int pipe(int fds[2]);` — `<unistd.h>`
Unidirectional channel: `fds[0]` read end, `fds[1]` write end.
In webserv: one to send the body to the CGI and one to read its
output. A pipe CAN block, so it must be non-blocking and watched by
`poll`. Always close the ends you do not use: if you keep the write
end open, you will never see the EOF that marks the end of the CGI
response.

### socketpair
```
int socketpair(int domain, int type, int protocol, int sv[2]);
```
`<sys/socket.h>` — A pair of connected, BIdirectional sockets. An
alternative to using two pipes to talk to the CGI.

### dup / dup2
```
int dup(int oldfd);
int dup2(int oldfd, int newfd);
```
`<unistd.h>` — Duplicate a fd. `dup2` forces the target number,
closing whatever was there first.
In webserv: in the child, `dup2(in[0], STDIN_FILENO)` and
`dup2(out[1], STDOUT_FILENO)` so the script reads the body from stdin
and writes its response to stdout.

### chdir
`int chdir(const char *path);` — `<unistd.h>`
Changes the process working directory.
In webserv: the subject requires it — the CGI must run in the script's
directory so its relative paths work. Do it in the CHILD, never in the
parent (that would affect the whole server).

### waitpid
```
pid_t waitpid(pid_t pid, int *status, int options);
```
`<sys/wait.h>` — Waits for a child's state change and collects its exit
status (`WIFEXITED`, `WEXITSTATUS`, `WIFSIGNALED`).
In webserv: use `WNOHANG` — blocking the process while waiting for the
CGI means the server stops serving everyone else.

### kill
`int kill(pid_t pid, int sig);` — `<signal.h>`
Sends a signal to a process.
In webserv: killing a CGI that exceeded its timeout (SIGKILL) so you
can answer 504.

### signal
```
void (*signal(int signum, void (*handler)(int)))(int);
```
`<csignal>` — Installs a signal handler.
In webserv: `signal(SIGPIPE, SIG_IGN)` is close to mandatory (writing
to a closed socket would kill the process), plus `SIGINT` for a clean
shutdown. Keep handlers minimal: only touch a `volatile
sig_atomic_t`.

---

## 5. Files and directories

### open
`int open(const char *path, int flags, ... mode_t mode);`
`<fcntl.h>` — Opens or creates a file and returns a fd.
In webserv: serving static files, storing uploads
(`O_CREAT|O_WRONLY|O_TRUNC` with mode 0644), creating temp files.

### read / write
```
ssize_t read(int fd, void *buf, size_t n);
ssize_t write(int fd, const void *buf, size_t n);
```
`<unistd.h>` — Generic I/O on any fd.
`read` returning 0 means EOF; both may transfer fewer bytes than
requested, so always loop with an offset. On sockets and pipes, only
after `poll` reports readiness.

### close
`int close(int fd);` — `<unistd.h>`
Releases the descriptor. Every `accept`, `open`, `pipe` and `dup` needs
its matching `close`, or you will exhaust the process fd table
(EMFILE) and the server will stop accepting connections.

### fcntl
`int fcntl(int fd, int cmd, ... );` — `<fcntl.h>`
Manipulates descriptor properties.
In webserv: `fcntl(fd, F_SETFL, O_NONBLOCK)` on every socket and pipe,
and `FD_CLOEXEC` so the server's fds do not leak into the CGI. Only
those flags are allowed.

### access
`int access(const char *path, int mode);` — `<unistd.h>`
Checks the real user's permissions: `F_OK` (exists), `R_OK`, `W_OK`,
`X_OK`.
In webserv: telling 404 (does not exist) apart from 403 (exists but no
permission), and verifying a CGI script is executable.

### stat
`int stat(const char *path, struct stat *buf);` — `<sys/stat.h>`
Fills in file metadata.
In webserv: `S_ISDIR(buf.st_mode)` to choose between index file,
autoindex or 403; `st_size` for `Content-Length`; `st_mtime` for
listings.

### opendir / readdir / closedir
```
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
```
`<dirent.h>` — Walk the contents of a directory. `readdir` returns NULL
when done; `d_name` holds the entry name.
In webserv: building the autoindex (directory listing) HTML.

---

## 6. Errors

### errno
`<cerrno>` — Global (thread-local) value holding the last error code.
FORBIDDEN after `read`/`write`/`recv`/`send` to decide what the server
does. The practical consequence: if `recv` returns -1 you close the
connection, full stop; you do not test for EAGAIN. That is why the
correct design is "one read per poll event".

### strerror
`char *strerror(int errnum);` — `<cstring>`
Turns an error code into readable text.
In webserv: start-up messages (`bind: Address already in use`).

### gai_strerror
`const char *gai_strerror(int ecode);` — `<netdb.h>`
Same idea for the codes returned by `getaddrinfo`, which does NOT use
errno (EAI_NONAME, EAI_AGAIN...).

---

## 7. Summary table

| Function          | What you use it for in webserv              |
|-------------------|---------------------------------------------|
| socket            | Create the listening fd                     |
| setsockopt        | SO_REUSEADDR before bind                    |
| bind              | Attach the socket to interface:port         |
| listen            | Switch the socket to passive mode           |
| accept            | Get the fd of a new client                  |
| connect           | Only for your own testers                   |
| send / recv       | Socket I/O after readiness                  |
| getsockname       | Find out the real local address             |
| getaddrinfo       | Resolve host/service from the config        |
| freeaddrinfo      | Free the previous result                    |
| getprotobyname    | Get the TCP protocol number                 |
| htons/htonl       | Port and address to network order           |
| ntohs/ntohl       | Back to host order                          |
| select            | Multiplex (1024 fd limit)                   |
| poll              | Multiplex (recommended option)              |
| epoll_*           | Multiplex on Linux, scalable                |
| kqueue/kevent     | Multiplex on macOS/BSD                      |
| fork              | Spawn the CGI process                       |
| execve            | Run the interpreter with the CGI env        |
| pipe              | Parent <-> CGI channel                      |
| socketpair        | Bidirectional alternative to a pipe         |
| dup / dup2        | Redirect the CGI's stdin/stdout             |
| chdir             | Run the CGI in its own directory            |
| waitpid           | Reap the child with WNOHANG                 |
| kill              | Abort a CGI that timed out                  |
| signal            | Ignore SIGPIPE, clean shutdown              |
| open              | Static files and uploads                    |
| read / write      | Generic I/O (disk files exempt from poll)   |
| close             | Avoid descriptor leaks                      |
| fcntl             | O_NONBLOCK and FD_CLOEXEC (nothing else)    |
| access            | Tell 403 apart from 404                     |
| stat              | Directory vs file, Content-Length           |
| opendir/readdir   | Build the autoindex                         |
| closedir          | Close the DIR stream                        |
| errno             | Only outside read/write paths               |
| strerror          | Error text                                  |
| gai_strerror      | getaddrinfo error text                      |

---

## 8. Questions to check you have really internalized this

1. Why does the fd returned by `accept` not inherit `O_NONBLOCK`?
2. If you cannot inspect `errno` after `recv`, how do you tell "no
   more data" from "the client closed"?
3. What happens if the parent does not close the write end of the CGI
   pipe before reading its output?
4. Why can `send` return fewer bytes than you asked for, and what
   should your server do in that case?
5. What is the difference between `POLLHUP` and `recv` returning 0?
6. Why would calling `chdir` in the parent break the whole server?
