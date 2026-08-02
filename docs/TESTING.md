# Testing

Guide for **running** and **creating** the Webserver tests. Two layers:

- **Unit tests** (C++) — parsers and pure logic, no network, no server.
- **Integration tests** (bash + `curl`/`nc`) — against the real server.

No external test framework (e.g. Google Test) is allowed: unit tests use a
tiny in-house harness; integration tests use standard shell tools.

## Running the tests

```sh
make test                # run unit + integration
make test_unit           # run unit tests only (no server, no network)
make test_integration    # run integration tests (starts a server instance)
make re                  # clean build first, to avoid stale objects
```

Both targets must exit non-zero on failure (so CI/pre-merge checks can rely
on them). Run `make re` in a clean tree, then `make test`, before every
merge.

For debugging a single failing case:

```sh
./tests/unit/config_parser_test      # run one unit binary directly
bash tests/integration/01_get.sh     # run one integration scenario
./webserv tests/fixtures/config/basic.conf   # start the server by hand
```

## Test layout

```
tests/
├── unit/                    # C++ unit test binaries, one per module
│   ├── config_parser_test.cpp
│   ├── http_parser_test.cpp
│   └── ...
├── integration/             # shell scenarios, numbered in order
│   ├── 01_get.sh
│   ├── 02_post.sh
│   └── ...
├── fixtures/                # inputs shared by the tests
│   ├── config/              #   sample server configs
│   ├── files/               #   static files to serve
│   └── cgi/                 #   scripts for CGI scenarios
└── run_tests.sh             # (optional) shared runner for integration
```

Unit test file naming mirrors the module: `config_parser_test.cpp` tests
`src/config/config_parser.cpp`. Integration scenarios are numbered so the
runner executes them in a deterministic order.

## Unit tests

A minimal harness in `tests/unit/test.h` (no external framework):

```cpp
#ifndef TEST_HPP
#define TEST_HPP

#include <iostream>
#include <string>

static int failures_ = 0;

#define TEST(name)                                                        \
    static void name();                                                   \
    static bool name##_registered = test::registerTest(#name, name);      \
    static void name()

#define ASSERT_EQ(actual, expected)                                       \
    if (!((actual) == (expected))) {                                      \
        ++failures_;                                                      \
        std::cerr << "FAIL " << __FILE__ << ":" << __LINE__               \
                  << "  " #actual " != " #expected << std::endl;          \
    }

#define ASSERT_TRUE(expr) ASSERT_EQ(expr, true)
#endif
```

What to test:

- **Config parser**: valid/invalid directives, unknown keys, defaults,
  `listen`/`server_name`/`root`/`index`/`error_page`/`client_max_body_size`,
  location blocks and method restrictions.
- **HTTP parser**: request line, header parsing, `Host` requirement,
  `Content-Length`, chunked bodies, header size/count limits, unknown
  methods/versions.
- **MIME / path helpers**: extension → content type, path joining, trailing
  slashes.

Rules:

- Unit tests exercise **pure logic only** — no sockets, no `poll()`. Anything
  needing the network belongs in integration tests.
- Each `TEST(name)` function covers one behaviour; the runner reports
  failures and the exit code must be non-zero on any failure.
- Register every test in the per-file `main()` (see the harness skeleton in
  `tests/unit/`).

## Integration tests

A scenario is a bash script: **fixture → start server → request → assert →
teardown**.

Template:

```sh
#!/usr/bin/env bash
set -euo pipefail
PORT="${PORT:-8080}"
BIN="${BIN:-./webserv}"

"$BIN" tests/fixtures/config/basic.conf &
SERVER_PID=$!
trap 'kill $SERVER_PID 2>/dev/null' EXIT
sleep 0.2

code=$(curl -s -o /dev/null -w "%{http_code}" "http://localhost:$PORT/")
[ "$code" = "200" ] || { echo "FAIL: expected 200, got $code"; exit 1; }

curl -s "http://localhost:$PORT/index.html" | grep -q "<title>Home</title>"
```

Common assertions:

```sh
# status code only
code=$(curl -s -o /dev/null -w "%{http_code}" "$url")

# exact body
curl -s "$url" | cmp -s - tests/fixtures/files/expected.txt

# headers
curl -sI "$url" | grep -qi '^content-type: text/html'

# raw HTTP with nc (checking exact bytes)
printf 'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc localhost "$PORT" | head -1 | grep -q '^HTTP/1.1 200'
```

What to cover (checklist by module):

- **Server / routing**: `GET` static file, `GET /` → index, unknown path →
  `404`, forbidden method → `405`, directory listing (`autoindex on/off`).
- **Methods**: `POST` with body / upload, `DELETE` existing/missing file,
  method not allowed per location.
- **Config**: multiple servers on different ports, `server_name` matching,
  `error_page` custom pages, redirects (`return`), `client_max_body_size` →
  `413`.
- **HTTP edge cases**: keep-alive (two requests one connection), chunked
  body, `Expect: 100-continue`, oversized headers → `431`/`400`, unknown
  version → `505`.
- **CGI**: `GET` with `QUERY_STRING`, `POST` body, script writing headers,
  missing interpreter → `500`.
- **Robustness**: malformed request → `400` and server still alive; idle
  timeout → `408`; stress with `siege`/`ab` (see
  [CHEATSHEET.md](CHEATSHEET.md#load--stress)).

## Quality gates

Before merging to `main`:

1. `make re` in a clean tree (no stale objects).
2. `make test` — full suite passes.
3. Valgrind on the server + a request (`valgrind --leak-check=full
   ./webserv ...`) → no leaks. ASan (`-fsanitize=address`) as a dev-only
   alternative.
4. New module ⇒ matching unit test in `tests/unit/` and (if it touches the
   wire) an integration scenario.
