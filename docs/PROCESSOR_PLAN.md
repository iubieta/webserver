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

- [ ] RFC 9110, section 9 (methods): GET, HEAD, POST, DELETE.
- [ ] RFC 9110, section 15 (status codes): skim classes, read the
      ones in your error list (301/302, 400, 403, 404, 405, 408, 413,
      500, 501, 505).
- [ ] RFC 9112: message format (request line, headers, body framing).

Skim only. Deep reading happens later, when a question sends you back.
RFC 7230-7235 and 2616 are obsolete; 9110/9112 replace them.

## Step 2 — NGINX lab

- [ ] Run NGINX in Docker (`nginx:stable`), config and `www/` mounted
      as volumes so edits reload fast.
- [ ] One config, several locations, one per behaviour to study:
      redirect, restricted methods, autoindex on, index file, custom
      `error_page`, a nested location (`/` and `/images`).
- [ ] A `www/` tree with files, subdirs, a dir without index, a file
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

## Step 5 — Contracts

- [ ] Agree the `Request` interface with the parser owner: fields and
      accessors only (method, target, version, headers, body).
- [ ] Draft `Decision` v1: server, location, physical path, handler
      kind, status, extra data (redirect target, allowed methods).
- [ ] Leave room in `Decision` for "not ready yet" (CGI, week 7).

Ask yourself: can every row of the table be expressed as a field of
`Decision`? If a row cannot, the struct is missing something.

## Step 6 — Basic processor

Minimum viable resolution:

- [ ] Server selection (listener default first; `Host` if implemented).
- [ ] Location matching by longest prefix, respecting path boundaries.
- [ ] `root` substitution into a physical path.
- [ ] Method check → 405.
- [ ] `stat()`: file, directory or missing.
- [ ] Unit tests with hand-built `Request` and config. No sockets, no
      parser dependency.

Test first the cases from the log that surprised you.

## Step 7 — Full processor

- [ ] `return` (redirects).
- [ ] `index` / `autoindex` decision for directories.
- [ ] `client_max_body_size` → 413.
- [ ] CGI detection by extension (decision only, no execution).
- [ ] `error_page` lookup, with inheritance server → location.
- [ ] Path traversal (`..`) cannot escape `root`.
- [ ] Unit tests for every new branch.

## Done when

- Every case in `NGINX_CASES.md` has a matching unit test.
- The processor never calls `open()`.
- A `Decision` fully describes what the builder must do, with no
  second look at the config.
