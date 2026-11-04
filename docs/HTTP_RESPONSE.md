# HTTP/1.1 Response: Standards and Message Structure

## 1. Introduction

An HTTP response communicates the result of a client's request. It contains a status code, metadata expressed as header fields, and, when permitted, a message body containing resource data or an explanation of the result.

This document describes HTTP/1.1 responses for the Webserv project. It combines the supplied structure and normative notes into one technical reference. The textual wire format described here applies to HTTP/1.1; HTTP/2 and HTTP/3 use different message framing.

Building a response means turning a request outcome into a correctly formatted sequence of bytes. For example, serving a file requires a suitable status code, representation metadata, a valid message boundary, and the file's bytes. An error also produces an HTTP response, with its own status and optional explanatory content.

## 2. Relevant RFCs and Protocol Requirements

### 2.1. Reference documents

| RFC | Title | Role in response construction |
| --- | --- | --- |
| [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html) | HTTP Semantics | Defines methods, status codes, field meanings, and content semantics. |
| [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html) | HTTP/1.1 | Defines message syntax, framing, and connection management. |

Useful sections include RFC 9110 Sections 6.6.1 (`Date`), 8.3 (`Content-Type`), 8.6 (`Content-Length`), 9.3.2 (`HEAD`), and 15 (status codes); and RFC 9112 Sections 2.1 (message format), 4 (status line), 5 (field syntax), 6 (message body), and 9.3 (persistence).

The requirements below use **MUST**, **MUST NOT**, **SHOULD**, and **MAY** in their normative sense: mandatory behavior, prohibited behavior, recommended behavior, and permitted behavior, respectively.

### 2.2. Syntax and line endings

- The status line and every header field line MUST end with CRLF: `\r\n`.
- An additional CRLF terminates the header section.
- A sender MUST NOT place whitespace between a field name and its colon.
- The reason phrase may be empty; the status code carries the response semantics.

### 2.3. Body presence and framing

| Response context | Message body | Framing restrictions |
| --- | --- | --- |
| Response to `HEAD` | Not transmitted | If `Content-Length` is present, it describes the content that the corresponding `GET` would send. |
| `1xx` or `204` | Prohibited | `Content-Length` is prohibited. |
| `304 Not Modified` | Prohibited | If present, `Content-Length` must match the content size of the corresponding `200 OK` response. |
| Successful `CONNECT` (`2xx`) | Connection becomes a tunnel after the headers | Neither `Content-Length` nor `Transfer-Encoding` may be sent. |
| Other responses | Generally permitted, possibly empty; individual status codes can impose further restrictions | Select framing consistent with the response semantics. |

A sender MUST NOT send `Content-Length` together with `Transfer-Encoding`. For a basic response with a known body size and no transfer coding, `Content-Length` specifies the exact number of body bytes. It does not include the status line, headers, or separating empty line.

HTTP/1.1 also supports chunked transfer coding. In that case, the body is encoded as chunks, and the wire format includes chunk framing and potentially trailers. The example in this document uses a known length and no transfer coding.

### 2.4. Context-dependent header fields

| Context | Header | Requirement or purpose |
| --- | --- | --- |
| `405 Method Not Allowed` | `Allow` | Required; lists the methods currently supported by the target resource. |
| `401 Unauthorized` | `WWW-Authenticate` | Required; supplies at least one authentication challenge. |
| `201 Created` | `Location` | Can identify the primary resource created; not universally mandatory. |
| Single-part `206 Partial Content` | `Content-Range` | Required to describe the transmitted range. Multipart responses place this field in each body part instead. |
| Origin server with a clock | `Date` | Required for `2xx`, `3xx`, and `4xx`; permitted for `1xx` and `5xx`. A server without a clock MUST NOT generate it. |
| Server identification | `Server` | Optional; identifies server software. |

`Content-Type` describes the media type of the representation. It is useful for HTML, text, JSON, and binary resources, but is not a universally mandatory field in every response.

### 2.5. Connection management

If the server does not support persistent connections, it MUST send `Connection: close` in every response except informational `1xx` responses, then close the connection after transmitting the response.

Persistence requires unambiguous message boundaries. Where a body is allowed, `Content-Length` or chunked transfer coding can provide such boundaries. Closing the connection can delimit some response bodies, but that connection cannot then be reused. The server must also consume the complete request body or close the connection before interpreting subsequent incoming bytes as another request.

## 3. HTTP/1.1 Response Structure

### 3.1. Overall layout

A basic response consists of a status line, zero or more header field lines, an empty line, and an optional message body.

```text
+----------------------------------------------------------+
| 1. STATUS LINE                                           |
| HTTP/1.1 200 OK\r\n                                      |
+----------------------------------------------------------+
| 2. HEADER FIELDS                                         |
| Date: Wed, 04 Nov 2026 12:00:00 GMT\r\n                   |
| Server: webserv/1.0\r\n                                  |
| Content-Type: text/plain; charset=utf-8\r\n               |
| Content-Length: 12\r\n                                   |
| Connection: close\r\n                                   |
+----------------------------------------------------------+
| 3. EMPTY LINE                                            |
| \r\n                                                    |
+----------------------------------------------------------+
| 4. MESSAGE BODY (when permitted)                          |
| Hello World!                                             |
+----------------------------------------------------------+
```

### 3.2. Status line

```text
HTTP-version SP status-code SP [ reason-phrase ] CRLF
```

`SP` means one space, and brackets indicate an optional component. For example, `HTTP/1.1 200 OK` identifies the protocol version, a three-digit result code, and a human-readable reason phrase.

| Status class | Meaning | Example |
| --- | --- | --- |
| `1xx` | Informational | `100 Continue` |
| `2xx` | Successful | `200 OK` |
| `3xx` | Redirection | `301 Moved Permanently` |
| `4xx` | Client error | `404 Not Found` |
| `5xx` | Server error | `500 Internal Server Error` |

An informational response can precede the final response. Each response has its own status line and header section.

### 3.3. Header fields

Each field line has a name, a colon, a value, and a CRLF terminator. A conventional readable form is:

```text
Field-Name: field value\r\n
```

Headers describe the response and its content. In the diagram, `Content-Type` identifies plain text encoded as UTF-8, `Content-Length` declares a 12-byte body, and `Connection: close` announces closure after the response. `Date` uses HTTP-date formatting in GMT; the displayed timestamp is illustrative.

Field names are case-insensitive. Their order is generally not significant when the names differ. Repeated fields require field-specific handling; a response model should not assume that every field name can occur only once.

### 3.4. Empty line

The empty line ends the header section. The final header's CRLF followed by this additional CRLF produces the familiar boundary:

```text
Connection: close\r\n\r\n
```

For a body-bearing response, body bytes begin immediately after that boundary. For a response that cannot contain a body, the response ends there, subject to the tunnel behavior of successful `CONNECT`.

### 3.5. Message body

The body can contain HTML, JSON, text, image data, or other bytes. Its contents are not necessarily text and do not use the header section's line-based syntax.

In this example, `Hello World!` contains exactly 12 ASCII bytes. There is no trailing newline in the body. Appending a newline would change its length and require updating `Content-Length`.

### 3.6. Complete response example

The following notation makes every CRLF visible:

```text
HTTP/1.1 200 OK\r\n
Date: Wed, 04 Nov 2026 12:00:00 GMT\r\n
Server: webserv/1.0\r\n
Content-Type: text/plain; charset=utf-8\r\n
Content-Length: 12\r\n
Connection: close\r\n
\r\n
Hello World!
```

The physical display line breaks in this block are for readability; the shown CRLF sequences specify the transmitted line endings. The closing Markdown fence is not part of the body.

The response is a byte stream. One socket write is not guaranteed to transmit the entire message, and the client does not depend on matching socket read boundaries to these logical sections.
