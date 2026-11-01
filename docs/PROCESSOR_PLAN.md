# Processor plan

Route plan for the request processor: the stage between the parser and
the response builder. Belongs to week 4 of [ROADMAP.md](ROADMAP.md).
Estimated total: **6-8 days**, one person, in parallel with the parser.

```
bytes -> Request -> Decision -> Response -> bytes
       parser     processor    builder     serializer
                  ^^^^^^^^^
                  this plan
```

The processor **decides**, it does not translate. It crosses the parsed
`Request` with the config and answers: which server, which location,
which physical path, which handler, which status. No `open()`, only
`stat()`.

## Overview

| Step | Focus                 | Time     | Output                          |
| ---- | --------------------- | -------- | ------------------------------- |
| 1    | RFC skim              | 1-2 h    | Vocabulary, section bookmarks   |
| 2    | NGINX lab             | 0.5 day  | Reproducible reference setup    |
| 3    | Experiments           | 1-2 days | `NGINX_CASES.md` case log       |
| 4    | Resolution order      | 0.5 day  | Ordered directive table         |
| 5    | Contracts             | 0.5 day  | `Request` agreed, `Decision` v1 |
| 6    | Basic processor       | 2 days   | Core resolution + unit tests    |
| 7    | Full processor        | 1-2 days | All directives covered          |

Then: response builder.

## Step 1 — RFC skim

- [x] RFC 9110, section 9 (methods): GET, HEAD, POST, DELETE.
- [x] RFC 9110, section 15 (status codes): skim classes, read the
      ones in your error list (301/302, 400, 403, 404, 405, 408, 413,
      500, 501, 505).
- [x] RFC 9112: message format (request line, headers, body framing).

Skim only. Deep reading happens later, when a question sends you back.
RFC 7230-7235 and 2616 are obsolete; 9110/9112 replace them.

## Step 2 — NGINX lab

- [x] Run NGINX in Docker (`nginx:stable`), config and `www/` mounted
      as volumes so edits reload fast.
- [ ] One config, several locations, one per behaviour to study:
      redirect, restricted methods, autoindex on, index file, custom
      `error_page`, a nested location (`/` and `/images`).
- [x] A `www/` tree with files, subdirs, a dir without index, a file
      without read permission.

Note: NGINX has no `methods`; its closest equivalent is `limit_except`.
Translate the intent, not the syntax.

## Step 3 — Experiments

Tools: `curl -v` for normal requests, `nc`/`telnet` for malformed ones.

Log every case with a fixed format (it becomes an integration test):

```
### <short name>
Request:   GET /images/ HTTP/1.1
Location:  /images (autoindex on, no index)
Expected?: <your guess before running>
Status:    200
Headers:   Content-Type: text/html
Body:      yes, directory listing
Notes:     <anything surprising>
```

Writing your guess first is the point: the surprises are what you
learn from.

Questions the log must answer:

- [ ] `/imagesfoo` with locations `/` and `/images`: which one wins?
- [ ] Directory without trailing slash: 301, 404 or listing?
- [ ] Directory with no index and autoindex off: 403 or 404?
- [ ] `POST` to a location that only has `return`: 405 or 301?
- [ ] Body over the limit on a forbidden method: 405 or 413?
- [ ] `error_page` pointing to a missing file: what is served?
- [ ] 405: which headers come with it?
- [ ] `HEAD` on a file: headers, `Content-Length`, body?
- [ ] File without read permission: 403 or 404?
- [ ] URI with `..`: what happens?

Timebox: 2 days max. Remaining doubts are solved while coding, with
the same method.

## Step 4 — Resolution order

Build the directive table: directive, question it answers, result on
"failure". Then **order the rows** using the experiment log.

- [ ] Every directive from the config parser appears in the table.
- [ ] Order justified by at least one logged case where it matters.
- [ ] `error_page` placed outside the sequence: it is consulted when
      any step fails, it is not a step.
- [ ] Defaults written down for every directive.

This ordered table is the processor design. Keep it in the repo.

| Order | Step | Responsible | Action / Result | Notes |
| :-- | :-- | :-- | :-- | :-- |
| 1 | Server selection | Processor | Pick `ServerConfig` by `listen` (default = first server). If virtual host by `Host` implemented, apply here. | Evolve `findConfig()`. |
| 2 | Decode + normalize path | Processor (`resolvePath`) | `decodePath` + `normalizePath`. If after normalize escapes root -> `""` => `400 Bad Request`. | Already partially done. |
| 3 | Location matching | Processor | Longest prefix match respecting segment boundaries. If no location -> `404 Not Found`. | Covered by tests. |
| 4 | Redirects (`return`) | Processor | If location has `return` -> `Decision` with `handler = REDIRECTION`, `redir_url`, corresponding status (301/302/303/307/308). | Position relative to methods to be validated with NGINX experiments. |
| 5 | Allowed methods | Processor | Check against `location.getMethods()`. If not allowed -> `405 Method Not Allowed`, `handler = ERROR_PAGE`, fill `allowed_methods_` (`Allow` header). | **Critical:** case "body too large + method not allowed". Prioritize per NGINX log (validate in Step 3). |
| 6 | Body size limit (`client_max_body_size`) | Processor + Connection | **If `Content-Length`:** compare to `location->getClientMaxBodySize()` after location resolved. If > limit and limit != 0 -> `413 Payload Too Large`, `handler = ERROR_PAGE`, **do not read body**. **If `chunked`:** **do not** reject here. Keep `server_`/`location_` in `Decision` so `Connection` enforces limit while unchunking. `0` = no limit. | **Never trust header alone.** Real byte counter is mandatory. |
| 7 | Resource resolution (`stat`) | Processor | With `disk_path = location.root + target` call `stat()`. Distinguish file/directory/missing/no permission. Apply `index`, `autoindex` (on/off), CGI detection by extension (decision only). | **Never call `open()`**. |
| 8 | Final handler selection | Processor | Set `READ_FILE`/`LIST_DIR`/`WRITE_FILE`/`REMOVE_FILE`/`CGI`/`REDIRECTION`/`ERROR_PAGE`. Fill complete `Decision` with pointers + needed data. | `Decision` must be sufficient for builder without re-consulting config. |
| 9 | Custom error pages | Processor/Builder | `error_page` with inheritance `server -> location`. Consulted **outside** the sequence on failure. | Fallback, not a sequence step. |

Note: Parser validates `Content-Length` (format/overflow/conflict with chunked). Processor does early check when known. Connection does hard cap byte-by-byte for both `Content-Length` and chunked.

## Step 5 — Contracts

- [ ] Agree the `Request` interface with the parser owner: fields and
      accessors only (method, target, version, headers, body).
- [ ] Draft `Decision` v1: server, location, physical path, handler
      kind, status, extra data (redirect target, allowed methods).
- [ ] Leave room in `Decision` for "not ready yet" (CGI, week 7).

Ask yourself: can every row of the table be expressed as a field of
`Decision`? If a row cannot, the struct is missing something.

### Request (agreed with parser)

| Field | Type | Notes |
| :-- | :-- | :-- |
| `method`, `target` (path), `query`, `version` | `std::string` | Existing |
| `headers` | map/structure | Existing |
| `hasContentLength` | `bool` | Extracted and syntactically validated by parser |
| `contentLength` | `size_t` | Only valid if `hasContentLength`. Digits only, no `+`, no spaces, must fit in `size_t`. |
| `chunked` / `transferEncodingChunked` | `bool` | Detected from `Transfer-Encoding: chunked` |
| `body` / buffer | optional | **Must not** be parsed/read by the Processor. Parser only reads up to end of headers. |

Parser rules:
- Validate `Content-Length` format/overflow. If invalid -> `400 Bad Request`.
- If both `Content-Length` and `Transfer-Encoding: chunked` present: decide priority (commonly prefer `chunked` or return `400`). Validate against NGINX experiments.
- **Never** apply `client_max_body_size` in parser (location not known yet).

### Shared context (Decision)

Extend `Decision` to share resolved `server`/`location` to avoid `Connection` redoing location matching:

```cpp
// Forward declarations to avoid circular includes
class ServerConfig;
class LocationConfig;

class Decision {
private:
    e_status_code status_;
    e_handler handler_;
    
    std::string disk_path_;
    std::string redir_url_;
    std::vector<std::string> allowed_methods_;

    // Resolved context from Processor (pointer modifiable, pointed-to content const)
    const ServerConfig* server_;      // pointer modifiable, content const
    const LocationConfig* location_;  // pointer modifiable, content const
public:
    void setServer(const ServerConfig* s);
    void setLocation(const LocationConfig* l);
    const ServerConfig* getServer() const;
    const LocationConfig* getLocation() const;
};
```

- Use **`const T*`**: pointer is reassignable/copiable, pointed-to content is immutable. Configs loaded at start, owned by `ServerCore` in `std::vector<ServerConfig>`, valid for lifetime.
- If `server`/`location` not resolved (early errors), pointers may be `NULL`.
- Initialize to `NULL` in constructors; copy pointers in copy/assignment.

## Step 6 — Basic processor

Minimum viable resolution:

- [ ] Server selection (listener default first; `Host` if implemented).
- [ ] Location matching by longest prefix, respecting path boundaries.
- [ ] `root` substitution into a physical path.
- [ ] Method check → 405.
- [ ] `stat()`: file, directory or missing.
- [ ] Fill `server_`/`location_` in `Decision` with `const T*` pointers.
- [ ] Early check `Content-Length` -> `413 Payload Too Large` (`ERROR_PAGE`, do not read body). If `chunked`, pass limit via pointers without rejecting.
- [ ] Unit tests with hand-built `Request` and config. No sockets, no
      parser dependency.

Test first the cases from the log that surprised you.

## Step 7 — Full processor

- [ ] `return` (redirects).
- [ ] `index` / `autoindex` decision for directories.
- [ ] `client_max_body_size` → 413.
- [ ] Support `chunked`: pass `client_max_body_size` via `Decision`/pointers to `Connection` (no 413 in Processor).
- [ ] Design hard cap in `Connection`: count real bytes for `Content-Length` and `chunked` (defense in depth).
- [ ] CGI detection by extension (decision only, no execution).
- [ ] `error_page` lookup, with inheritance server → location.
- [ ] Path traversal (`..`) cannot escape `root`.
- [ ] Tests for `413` via direct `Content-Length` and `chunked` exceeding limit.
- [ ] Cover "method not allowed + body too large" per NGINX log.
- [ ] Unit tests for every new branch.

## Done when

- Every case in `NGINX_CASES.md` has a matching unit test.
- The processor never calls `open()`.
- Processor shares `const ServerConfig*`/`const LocationConfig*` in `Decision` (pointer modifiable, content `const`).
- Parser validates `Content-Length` format/overflow and detects conflict with `Transfer-Encoding: chunked`.
- `Connection` counts real bytes and applies hard cap for both `Content-Length` and `chunked`. If Processor already returns `413` for `Content-Length`, **do not** read body.
- Case `405` vs `413` covered with NGINX test/log.
- A `Decision` fully describes what the builder must do, with no
  second look at the config.

## Responsibilities split

| Component | Responsibility | Restrictions |
| :-- | :-- | :-- |
| **Parser (`HttpParser`/`Request`)** | Read only headers. Extract/validate `Content-Length`, detect `chunked`, detect conflict. Fill flags. | No `client_max_body_size`, no location matching, no body reading. |
| **Processor** | Server/location matching, path traversal, methods, early `Content-Length`->413, redirects, `stat()` + index/autoindex/CGI (decision only), fill `Decision` with `const` pointers. | **Never `open()`**. No body reading. Delegate streaming to IO. |
| **Connection / ServerCore** | Apply `Decision`. If `413` already returned -> skip body. If accepted or `chunked` -> read counting real bytes with hard cap via copied location pointer. If exceeds at any time -> `413`, stop reading. | **Never trust header blindly.** Byte counter mandatory. |

Key points: pointer modifiable, content `const` (`const T*`). `Content-Length` enables early reject when known; `chunked` requires streaming enforcement. Always count bytes actually received.
