# HTTP/1.1 quick reference

Focused reference for what the Webserver must parse, generate and handle.
Scope: **HTTP/1.1** as defined by RFC 7230–7235, restricted to the methods
required by the project.

## Request format

```
<method> <request-target> HTTP/<version>\r\n
<header>: <value>\r\n
...
\r\n
<body>
```

Example:

```
GET /index.html HTTP/1.1\r\n
Host: localhost\r\n
Connection: close\r\n
\r\n
```

- Every line ends with `\r\n`; the header block ends with an empty line.
- `Host` is required in HTTP/1.1.
- Target may be an absolute path (`/index.html`) or absolute URI
  (`http://host/index.html`).

## Response format

```
HTTP/<version> <status-code> <reason-phrase>\r\n
<header>: <value>\r\n
...
\r\n
<body>
```

Example:

```
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 13\r\n
Connection: close\r\n
\r\n
Hello world!
```

## Methods

| Method | Meaning                                     | Required by project |
| ------ | ------------------------------------------- | ------------------- |
| GET    | Retrieve the resource                       | yes                 |
| POST   | Submit data; may create a resource          | yes                 |
| DELETE | Remove the resource                         | yes                 |
| HEAD   | Same as GET, response without body          | no (nice to have)   |

## Status codes

### 1xx Informational
| Code | Reason     | Note                          |
| ---- | ---------- | ----------------------------- |
| 100  | Continue   | sent on `Expect: 100-continue`|
| 101  | Switching Protocols | (not used)            |

### 2xx Success
| Code | Reason   | Note                              |
| ---- | -------- | --------------------------------- |
| 200  | OK       | default success                   |
| 201  | Created  | after successful POST             |
| 204  | No Content | success, empty body             |

### 3xx Redirection
| Code | Reason          | Note                            |
| ---- | --------------- | ------------------------------- |
| 301  | Moved Permanently | needs `Location` header       |
| 302  | Found           | temporary redirect, `Location`  |
| 307  | Temporary Redirect | like 302, method preserved   |
| 308  | Permanent Redirect | like 301, method preserved   |

### 4xx Client error
| Code | Reason           | Note                              |
| ---- | ---------------- | --------------------------------- |
| 400  | Bad Request      | malformed syntax                  |
| 403  | Forbidden        | no permission / method not allowed|
| 404  | Not Found        | resource missing                  |
| 405  | Method Not Allowed | `Allow` header required         |
| 408  | Request Timeout  | idle client                       |
| 411  | Length Required  | `Content-Length` missing when needed |
| 413  | Payload Too Large| body exceeds limit                |
| 415  | Unsupported Media Type | unsupported `Content-Type`  |
| 431  | Request Header Fields Too Large | header size/count limit exceeded |

### 5xx Server error
| Code | Reason                  | Note                        |
| ---- | ----------------------- | --------------------------- |
| 500  | Internal Server Error   | CGI failure, unhandled error|
| 501  | Not Implemented         | unsupported method          |
| 503  | Service Unavailable     | overloaded, shutting down   |
| 505  | HTTP Version Not Supported | not HTTP/1.1             |

## Key headers

### Request
| Header             | Purpose                                 |
| ------------------ | --------------------------------------- |
| `Host`             | target host (required)                  |
| `Content-Length`   | body size in bytes                      |
| `Transfer-Encoding`| e.g. `chunked` (body framing)           |
| `Content-Type`     | media type of the body                  |
| `Connection`       | `keep-alive` or `close`                 |
| `Expect`           | `100-continue` support                  |
| `Accept`           | preferred response media types          |
| `Authorization`    | credentials (may be ignored)            |

### Response
| Header             | Purpose                                 |
| ------------------ | --------------------------------------- |
| `Content-Type`     | media type of the body                  |
| `Content-Length`   | body size in bytes                      |
| `Connection`       | `keep-alive` or `close`                 |
| `Location`         | redirect target (3xx)                   |
| `Allow`            | methods permitted (405)                 |
| `Date`             | response timestamp (HTTP/1.1 requires it)|
| `Server`           | server identification string            |

## Edge cases to handle

- **Chunked transfer**: parse `Transfer-Encoding: chunked` bodies
  (size lines in hex, `0` chunk + final `\r\n` terminates).
- **Keep-alive**: multiple requests per connection; use `Content-Length`
  to know where a request ends.
- **`Expect: 100-continue`**: send `HTTP/1.1 100 Continue` before reading
  the body.
- **Missing `Content-Length`** on POST: respond `411 Length Required` or
  rely on `Transfer-Encoding`.
- **Unknown method / version**: `501 Not Implemented` / `505 HTTP Version
  Not Supported`.
- **Absolute URI** request-target: strip the scheme/authority, keep the path.
- **Header size / count limits**: cap buffers to prevent abuse → `431` /
  `400`.
- **Persistent connection timeout**: close idle connections (`408`).
