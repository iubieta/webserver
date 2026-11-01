# HttpParser

The Request Parser is responsible for interpreting and transforming the raw 
binary stream from the network into a logical, structured representation of 
the request.

## HttpRequest Tasks

Since network operations are non-blocking and data may arrive in fragments 
across multiple calls to `recv()` or `read()`, the parser operates using a 
finite-state machine that processes the input byte by byte in an incremental 
manner:

1. Reception and Increment:
    Receives the data buffer sent by the Server Core and adds it to an internal
    accumulative buffer.

2. Request Line Analysis:
    Reads until it finds the first line delimited by CRLF (\r\n). Splits the 
    line into three main tokens using spaces as delimiters: Method (e.g., GET,
    POST), URI/Path (e.g., /index.html), and HTTP Version (e.g., HTTP/1.1).

3. Header Analysis:
    Processes the key-value pairs separated by colons (Header-Name: Value) line
    by line; these are also terminated by CRLF. Detects the final blank line 
    (\r\n\r\n), which marks the definitive end of the header section.

4. Determining the Body Strategy:
    Inspects the extracted headers to verify whether a message body exists:
    If Content-Length is found, extracts the total number of expected bytes.
    If Transfer-Encoding: chunked is found, activates the logic to decode 
    hexadecimal chunks.

5. Reading and Assembling the Body:
    If the request includes a body (common in POST or PUT requests), accumulate
    the received bytes until the number specified by `Content-Length` is 
    reached or until the final zero-size chunk (0\r\n\r\n) is received.

6. Syntax Validation and Error Handling:
    If the protocol syntax is violated in any of the steps (e.g., malformed 
    lines, overly long URIs, or invalid headers), it sets internal error flags,
    stops parsing, and sets the corresponding status code (e.g., 400 Bad 
    Request).

# Request-target

The request line (*request-line*) has the syntax `method SP target SP version 
CRLF`. The **request-target** identifies the target resource on which the 
client wishes to perform the action.

There are several ways to parse the request-target; the methods to be used for 
the web server are as follows:

##  `origin-form`

It is the most common format and the standard used on the Web when a client 
communicates **directly with the origin server**

* **Syntx:** `origin-form = absolute-path [ "?" query ]`.
* **Structure:** It contains only the absolute path that begins with a slash 
`/` (`absolute-path`) and, optionally, the query parameters (`query string`) 
that follow the `?` character. **It does not include** the scheme (`http://`) 
or the domain name.

**Examples**:

1.  `GET /mypage.html HTTP/1.1`
  * path: `/mypage.html`
  * query : **(empty)**

2. `GET /api/upload?user=juan&amp;type=pdf HTTP/1.1`
  * path: `/api/upload`
  * query: `user=juan&amp;type=pdf`

## 2\. `absolute-form`

This is the format in which the client sends the **complete, integrated URI**, 
including the protocol scheme (`http://` or `https://`) and the host or domain 
name.


**Syntax:** `absolute-form = absolute-URI`.

**When is it used?**: 

It is generated automatically when the client makes requests through a 
**proxy server**. However, the specification requires that normal origin 
servers **must accept** the absolute form for compatibility and robustness 
reasons.

**Examples**:

* `GET http://www.example.org/pub/WWW/TheProject.html HTTP/1.1`
* `GET http://proxy.local:8080/index.html HTTP/1.1`
* `POST https://api.servicio.com/v1/data?token=xyz HTTP/1.1`

# diagram

                     ┌───────────────────────────┐
                     │        HttpRequest        │
                     └─────────────┬─────────────┘
                                   │
                         It has a syntax error?
                                ┌──┴──┐
                             yes│     │ Not
                                ▼     ▼
                 ┌──────────────────┐ ┌─────────────────────────┐
                 │ Response Builder │ │   business logic        │
                 │ (error page)│    │ |  (CGI / Static Archive) │
                 └──────────────────┘ └────────────┬────────────┘
                                                   │
                                                   ▼
                                      ┌──────────────────┐
                                      │ Response Builder │
                                      │  (200 OK / etc.) │
                                      └──────────────────┘

# Structure of the Petition

```text
+---------------------------------------------------+
|                   REQUEST LINE                    |
| [METHOD] [SP] [URI/PATH] [SP] [VERSION] [CRLF]    |
| Ex: POST  " " /api/upload  " "  HTTP/1.1  \r\n    |
+---------------------------------------------------+
|                      HEADERS                      |
| [Header-Name]: [Header-Value] [CRLF]              |
| Host: localhost:8080\r\n                          |
| Content-Type: application/x-www-form-urlcoded\r\n |
| Content-Length: 15\r\n                            |
+---------------------------------------------------+
|              SEPARATOR (Empty Line)               |
| \r\n                                              |
+---------------------------------------------------+
|                   MESSAGE BODY                    |
| [Raw Data / Text / Form / Binary / Chunks]        |
| Ex: name=John&age=30                              |
+---------------------------------------------------+
```

# State Management in the HTTP Parser

```
```text
  [ Raw Network Buffer ]
            │
            ▼
┌───────────────────────┐
│  STATE_REQUEST_LINE   │ ──(Syntax Error)──┐
└───────────┬───────────┘                   │
            │ (\r\n)                        │
            ▼                               ▼
┌───────────────────────┐           ┌───────────────┐
│     STATE_HEADERS     │ ──(Error)─►│  STATE_ERROR  │
└───────────┬───────────┘           └───────────────┘
            │ (\r\n\r\n)                    ▲
            ▼                               │
┌───────────────────────┐                   │
│      STATE_BODY       │ ──(Too Large)─────┘
└───────────┬───────────┘
            │ (Done / 0\r\n\r\n)
            ▼
┌───────────────────────┐
│    STATE_COMPLETE     │ ──► [ Business Logic ]
└───────────────────────┘
```

```

---