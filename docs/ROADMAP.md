# Roadmap

Project milestones for the Webserver. **10 weeks**, two people, full-time.
Tests are continuous (see [TESTING.md](TESTING.md)) — every week ships with
its tests; week 5 is a mid-project hardening checkpoint before dynamic
responses.

## Overview

| Week | Focus               | Deliverable                                        |
| ---- | ------------------- | -------------------------------------------------- |
| 0    | Design              | Class/interfaces skeleton, error codes, config grammar, MIME list |
| 1    | Event loop + Logger | Single `poll()`, non-blocking, working logger, basic stress |
| 2    | Config + model      | Config parser with all directives, multi-server + default, unit tests |
| 3    | HTTP parser         | Request/headers/body, chunked, 100-continue, limits (400/431), unit tests |
| 4    | Static responses    | GET, MIME, index, autoindex, error_page, redirects, keep-alive, integration tests |
| 5    | **Mid hardening**   | Edge cases review, malformed→400, timeouts, valgrind, architecture review |
| 6    | Dynamic responses   | POST + upload, 413, DELETE (204/404), tests |
| 7    | CGI                 | fork/execve, env vars, pipe, script headers, 500, tests |
| 8    | Final hardening     | Remaining edge cases (501/505, dropped connections), siege/ab, leaks, coverage |
| 9    | Delivery            | Buffer 2-3 days, clean `make re` + `make test`, valgrind, example config |

## Week 0 — Design

- [ ] Re-read the subject and list mandatory requirements (config directives, poll, methods, CGI).
- [ ] Define classes/interfaces per module (`config`, `http`, `server`, `cgi`).
- [ ] Define error codes and status enum.
- [ ] Config grammar: directives and defaults.
- [ ] Initial MIME extension → content-type list.
- [ ] Header skeleton in `inc/`.

Do not over-design: the design is validated while coding. Max 3-4 days.

## Week 1 — Event loop + Logger

- [ ] Non-blocking sockets (`O_NONBLOCK`).
- [ ] Single `poll()` for all I/O (listen included), read and write together.
- [ ] `accept`/`read`/`write` only via `poll()`, handle `EAGAIN`/`EWOULDBLOCK`.
- [x] Logger per [LOGGING.md](LOGGING.md): stderr + file, levels
      (`Logger::Level`), per-output thresholds.
- [ ] Access log line on response (pending design, see
      [LOGGING.md](LOGGING.md#no-implementado-access-log)).
- [ ] Basic robustness: `nc`/`ab` with concurrent connections.

The event loop is the heart of the project — invest the time here.

## Week 2 — Config + data model

- [ ] Parser for `listen`, `server_name`, `root`, `index`, `error_page`,
      `client_max_body_size`, `location`, `methods`, `autoindex`, `return`.
- [ ] Validation of invalid directives and defaults.
- [ ] `ServerConfig` / `LocationConfig` model.
- [ ] Multiple servers; first server is the default for unassigned requests.
- [ ] Unit tests: `tests/unit/config_parser_test.cpp`.

## Week 3 — HTTP parser

- [ ] Request line (method, target, version).
- [ ] Headers, `Host` required, header limits → `400`/`431`.
- [ ] Body via `Content-Length`.
- [ ] Chunked transfer encoding.
- [ ] `Expect: 100-continue`.
- [ ] Absolute URI request targets.
- [ ] Unit tests: `tests/unit/http_parser_test.cpp`.
- [ ] Minimal hardcoded response to close the parse→send loop.

## Week 4 — Static responses

- [ ] Static `GET`: MIME lookup, `Content-Length`.
- [ ] `index` resolution for directories.
- [ ] `autoindex` on/off (directory listing).
- [ ] `error_page` custom pages.
- [ ] Redirects (`return` directive).
- [ ] Keep-alive: `Connection` header, multiple requests per connection.
- [ ] Routing: pick server/location by `Host` + URI.
- [ ] Integration tests: `curl`/`nc` for endpoints, status, bodies.

## Week 5 — Mid-project hardening

Checkpoint before dynamic responses: the server must be solid so far.

- [ ] Review accumulated HTTP edge cases.
- [ ] Malformed requests → `400`, server stays alive.
- [ ] Idle timeouts → `408`.
- [ ] First `valgrind`/ASan pass on server + requests.
- [ ] Architecture review of weeks 1-4 (fix debt before building on it).
- [ ] Coverage of weeks 1-4 tests.

## Week 6 — Dynamic responses

- [ ] `POST` with body (`Content-Length` / chunked).
- [ ] Upload to disk.
- [ ] `client_max_body_size` → `413`.
- [ ] `DELETE`: existing file → `204`, missing → `404`.
- [ ] Integration tests for POST/DELETE/upload.

## Week 7 — CGI

- [ ] `fork`/`execve` for CGI only.
- [ ] Environment vars: `REQUEST_METHOD`, `QUERY_STRING`,
      `CONTENT_LENGTH`, `PATH_INFO`, `SCRIPT_NAME`, etc.
- [ ] Pipe CGI output back to the client.
- [ ] Parse the script's own headers.
- [ ] Script/interpreter failures → `500`.
- [ ] Integration tests: GET with query, POST body, failure → 500.

## Week 8 — Final hardening

- [ ] Remaining edge cases: unknown method/version → `501`/`505`, dropped
      connections, oversized headers/bodies.
- [ ] Stress with `siege`/`ab`.
- [ ] Leaks: `valgrind`/ASan, closed file descriptors.
- [ ] Full coverage of the [TESTING.md](TESTING.md) checklist.
- [ ] Real multi-server setup: several ports, `server_name` matching.

## Week 9 — Delivery

- [ ] Keep 2-3 days of buffer for anything that slipped.
- [ ] Clean tree: `make re`, then `make test` passes.
- [ ] Final `valgrind` run, no leaks.
- [ ] Example config + README polish.
- [ ] Review commits against [CONVENTIONS.md](CONVENTIONS.md).
