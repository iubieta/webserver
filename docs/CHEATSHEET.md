# Usage cheatsheet

Quick reference for building, running and testing the Webserver.

## Build

```sh
make            # build the binary (./webserv)
make clean      # remove object files
make re         # clean then rebuild
make fclean     # remove objects + binary
```

The binary is produced in the repo root as `webserv`.

## Run

```sh
./webserv                      # default config
./webserv config/default.conf  # explicit config file
```

On startup the server prints the listening sockets and blocks until
`SIGINT`/`SIGTERM`.

## Configuration file

Plain-text, one `directive;` per line (see `config/` for examples).

```txt
server {
    listen 8080;
    server_name localhost;
    root ./www;
    index index.html;
    error_page 404 ./res/404.html;
    location /cgi-bin {
        cgi_pass /usr/bin/python3;
    }
}
```

## Manual testing

### curl

```sh
# basic GET
curl -v http://localhost:8080/

# GET with headers
curl -v http://localhost:8080/index.html -H "Host: localhost"

# POST with body
curl -v -X POST http://localhost:8080/upload -d "key=value"

# DELETE
curl -v -X DELETE http://localhost:8080/remove.txt

# download a file
curl -v http://localhost:8080/file.bin -o file.bin

# limit output (check status line only)
curl -s -o /dev/null -w "%{http_code}\n" http://localhost:8080/
```

### nc / telnet (raw HTTP)

```sh
printf 'GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n' | nc localhost 8080
telnet localhost 8080
```

### Load / stress

```sh
siege -c 25 -t 10s http://localhost:8080/
ab -n 1000 -c 50 http://localhost:8080/
```

## Tests

```sh
make test                # run unit + integration tests
make test_unit           # unit tests only
make test_integration    # integration tests (starts a server)
```

Test cases live in `tests/`. Each module should have a matching unit test
and, if it touches the wire, an integration scenario. Before merging, run
the full suite plus `make re` from a clean tree.

Full guide on running and writing tests: [TESTING.md](TESTING.md).

## Common checks when something breaks

1. `make re` and check for warnings.
2. Request against the wrong port/host? Verify `config/` and `server_name`.
3. CGI not running? Confirm `cgi_pass` path and file permissions.
4. Use `curl -v` and `nc` to inspect the exact bytes sent.
5. Confirm the event loop logs the connection (see startup output).
