# Status codes: scope and ownership

Which HTTP status codes this server can actually produce, and which
stage of the pipeline is responsible for each one.

```
bytes -> Request -> Decision -> Response -> bytes
       parser     processor    builder     serializer
```

Two questions, in this order:

1. **Scope:** can our server produce this code with what the subject
   requires? Most of the RFC fails this filter.
2. **Ownership:** which stage detects the condition that produces it?

## 1. Out of scope

These codes belong to features we do not implement.

| Code                    | Purpose                | Why it does not apply   |
| ----------------------- | ---------------------- | ----------------------- |
| 206, 416                | Ranges (`Range`)       | No partial serving      |
| 401                     | Authentication         | No users, no login      |
| 406, 415                | Content negotiation    | One format per resource |
| 410                     | "Existed, now gone"    | No memory of deletions  |
| 422, 423, 424, 507, 508 | WebDAV                 | Other HTTP extension    |
| 425, 426                | TLS / upgrade          | No TLS, no upgrade      |
| 429                     | Rate limiting          | No request limits       |
| 103, 300, 510, 511      | Very specific cases    | Nothing in the subject  |
| 418                     | An RFC joke            | -                       |

That leaves roughly 20 codes that matter.

## 2. Ownership of the codes in scope

| Stage            | Codes                          | Detects               |
| ---------------- | ------------------------------ | --------------------- |
| Parser           | 400, 413 (size), 414, 431,     | Protocol errors, no   |
|                  | 501, 505                       | config applied yet    |
| Processor        | 3xx (`return`), 403, 404,      | Rules from the config |
|                  | 405, 413 (config limit)        |                       |
| Handlers/builder | 200, 201, 204, 409, 500        | Result of disk I/O    |
| Event loop       | 408, 504                       | Time, not content     |
| CGI              | 502, 504                       | Bad output, no exit   |

`503` is optional (overload).

## 3. Explanations

**408 - Request Timeout**: The event loop, watching the
clock, sees that a client has sent nothing for X seconds. Same family
as 504.

**200, 201 and 204 Methods and Runtime errors**: The processor cannot
know if a method succeeded before running the handler.The handler tries, 
and the outcome is 201 or 500.

**409 - Conflicts:** belongs to the handlers. It signals a conflict 
with the *state* of the resource, e.g. DELETE on a non-empty directory. 
Knowing that requires looking at the disk.

**3xx - Redirects:** if `return` accepts any 3xx code, 
the processor emits whatever the config says, including 303, 307 and 308.

**500 - Internal server error:** can come from anywhere. 
Something fails unexpectedly, mostly the handlers.

**502 and 504 - Gateway errors:** these are CGI errors; invalid output 
and a script that never finishes, respectively.

## 4. The interesting case: 413 - Content too large

413 looks like a parser code: the parser reads `Content-Length`. But
the limit (`client_max_body_size`) can depend on the **location**, and
the processor resolves the location.

That forces an order:

```
headers complete -> processor -> read body
```

The processor must be able to run before the body arrives, not at the
end. If `Content-Length` already exceeds the limit, reply 413 without
reading the remaining 2 GB.

### 100 - Continue

Same principle. `curl` sends `Expect: 100-continue` for large bodies
and waits briefly before sending them. The server may answer `100`
("send it") or an error straight away (413, 405). Optional, but it
follows the same rule: decide after the headers, before the body.

### Open question for the parser owner

Does the parser hand over the `Request` in two phases (headers, then
body) or all at once? The 413 and 100 cases need the first option.
