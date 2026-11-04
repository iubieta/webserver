# CGI Invoking the Script

## 1. Purpose of This Document

This document explains how an HTTP server identifies, selects, prepares, and invokes a CGI script.

It is based on **RFC 3875 — The Common Gateway Interface (CGI) Version 1.1, Section 3: Invoking the Script**.

The objective is to understand the responsibilities of the server during CGI invocation, the relationship between the requested URI and the selected script, the Script-URI concept, and the execution process.

It also identifies the requirements of the Webserv project that affect CGI invocation.

---

## 2. Server Responsibilities

### 2.1 The Server as an Application Gateway

According to RFC 3875, the HTTP server acts as an application gateway.

It is responsible for coordinating the interaction between the HTTP client and the CGI program.

The general process consists of the following operations:

1. Receive the client request.
2. Select the CGI script that will handle the request.
3. Convert the client request into a CGI request.
4. Execute the selected script.
5. Convert the CGI response into a response for the client.

The server remains responsible for HTTP communication throughout this process.

Conceptually:

```text
             HTTP CLIENT
                  |
                  | HTTP Request
                  v
           +--------------+
           | HTTP SERVER  |
           +--------------+
                  |
                  | Select CGI
                  | Prepare CGI Request
                  v
           +--------------+
           | CGI PROGRAM |
           +--------------+
                  |
                  | CGI Response
                  v
           +--------------+
           | HTTP SERVER  |
           +--------------+
                  |
                  | HTTP Response
                  v
             HTTP CLIENT
```

The CGI program handles application-specific processing, but it does not replace the HTTP server.

### 2.2 Protocol and Transport Responsibilities

The server is responsible for implementing protocol-level and transport-level authentication and security.

It MUST perform the translations and protocol conversions required by the CGI specification.

Furthermore, the server retains its responsibility to the client to conform to the relevant network protocol even if the CGI script fails to conform to the CGI specification.

This means that the server cannot blindly forward an invalid CGI response to the client without handling the situation.

It must continue to manage the HTTP communication correctly.

### 2.3 Authentication and Access Control

RFC 3875 establishes an explicit requirement concerning authentication.

If the server applies authentication to a request, it MUST NOT execute the CGI script unless the request passes all defined access controls.

Conceptual flow:

```text
             HTTP Request
                  |
                  v
           +--------------+
           | HTTP SERVER  |
           +--------------+
                  |
                  v
          Access Control Check
                  |
             +----+----+
             |         |
          Denied    Authorized
             |         |
             v         v
        Reject      Execute CGI
        Request
```

This prevents unauthorized clients from invoking protected CGI programs.

### 2.4 Non-Transparent Server Behavior

RFC 3875 also allows the server to operate in a non-transparent manner.

This means that the server MAY modify the request or response to provide additional services.

Examples mentioned by the specification include:

- Media type transformation.
- Protocol reduction.

This behavior is optional and depends on the server implementation.

---

## 3. Script Selection

### 3.1 Identifying the CGI Script

The server determines which CGI script must be executed based on the generic-form URI supplied by the client.

The URI contains a hierarchical path whose components are separated by `/`.

The server identifies either:

- The entire path as belonging to an individual script.
- A leading part of the path as belonging to an individual script.

The remaining part, if any, identifies a resource or sub-resource that must be interpreted by the selected script.

This division is important because the script path and the additional resource path have different meanings.

### 3.2 Script Path and Additional Resource Path

Consider the following HTTP request:

```http
GET /cgi-bin/users.py/profile/25?format=json HTTP/1.1
Host: example.com
```

The URI path can be divided into two parts:

```text
/cgi-bin/users.py/profile/25
```

**Script path:**

```text
/cgi-bin/users.py
```

Identifies the CGI program that must be invoked.

**Additional resource path:**

```text
/profile/25
```

Identifies a resource or sub-resource that the CGI program must interpret.

**Query string:**

```text
format=json
```

Provides additional information to the CGI program.

The corresponding CGI meta-variables are:

```text
SCRIPT_NAME=/cgi-bin/users.py
PATH_INFO=/profile/25
QUERY_STRING=format=json
```

The selected program is `users.py`.

The additional path `/profile/25` does not necessarily represent part of the executable's filesystem path. It is information provided to the script so that the application can determine which resource to process.

For example, the script could interpret `/profile/25` as a request to retrieve the profile of the user identified by `25`.

### 3.3 Why This Separation Is Useful

The separation allows a single CGI program to handle multiple resources.

For example:

```text
/cgi-bin/users.py/profile/10
/cgi-bin/users.py/profile/25
/cgi-bin/users.py/profile/42
```

All three requests may invoke the same CGI program:

```text
/cgi-bin/users.py
```

Only the additional resource path changes.

This allows applications to expose different resources through a single CGI program without requiring a separate executable for every resource.

### 3.4 Hierarchical URI Requirement

RFC 3875 describes script selection using hierarchical URI paths.

Support for non-hierarchical URI schemes is outside the scope of this specification.

The server is responsible for determining where the script path ends and where the additional resource path begins.

The resulting information is made available to the script through CGI meta-variables.

---

## 4. The Script-URI

### 4.1 Mapping the Client URI to a CGI Script

RFC 3875 establishes that the mapping between the URI supplied by the client and the selected CGI script is determined by the particular server implementation and its configuration.

A server may allow the same script to be identified through several different URI path hierarchies.

For example, a particular CGI program might be accessible through:

```text
/cgi-bin/catalog.py
/tools/catalog
/apps/store/catalog.py
```

These URI paths could all identify the same CGI program, depending on the server configuration.

The specification allows the server to:

1. Preserve the URI supplied by the client.
2. Select a canonical URI from the possible URI values associated with the script.
3. Implement another selection strategy.

This means that the original client URI does not necessarily have to be the URI used internally when generating CGI meta-variables.

### 4.2 Definition of Script-URI

The **Script-URI** is a URI that can be constructed from the CGI meta-variables generated by the server.

It MUST satisfy the following condition:

If the client had accessed the Script-URI directly, the script would have been executed with the same values for:

- `SCRIPT_NAME`
- `PATH_INFO`
- `QUERY_STRING`

The Script-URI therefore represents an equivalent CGI invocation.

Its general structure is:

```text
scheme://server-name:server-port/script-path/extra-path?query-string
```

The components are:

| Component | Description |
|---|---|
| `scheme` | URI access scheme. |
| `server-name` | Server name. |
| `server-port` | Server port. |
| `script-path` | Path identifying the CGI script. |
| `extra-path` | Additional path interpreted by the script. |
| `query-string` | Query component of the URI. |

Example:

```text
http://example.com:8080/cgi-bin/users.py/profile/25?format=json
```

The relevant CGI meta-variables are:

```text
SCRIPT_NAME=/cgi-bin/users.py
PATH_INFO=/profile/25
QUERY_STRING=format=json
```

The Script-URI does not permit object parameters or fragment identifiers.

### 4.3 URL Encoding of Path Components

RFC 3875 specifies that the `SCRIPT_NAME` and `PATH_INFO` values are URL-encoded when used to construct the script-path and extra-path components of the Script-URI.

For this encoding, `;`, `=` and `?` are reserved.

This distinction matters because the values used internally by CGI must be represented correctly when reconstructing a URI.

### 4.4 Scheme and Protocol Are Not Identical

The specification also distinguishes between the URI scheme and the application protocol.

For example:

```text
https://example.com/cgi-bin/users.py
```

Here:

- `https` is the URI scheme.
- HTTP is the application protocol.
- TLS provides the secure transport.

CGI/1.1 does not provide a generic mechanism for reconstructing every possible URI scheme from the standard meta-variables.

The specification describes the Script-URI using the available protocol information and allows implementations to use scheme-specific meta-variables to determine the URI scheme more accurately.

This is an important distinction when reconstructing a complete URI.

---

## 5. Script Execution

### 5.1 Execution Mechanism

RFC 3875 specifies that the script is invoked in a **system-defined** manner.

Unless otherwise specified, the file containing the script is invoked as an executable program.

The exact execution mechanism is therefore dependent on the underlying system.

The CGI specification does not mandate a particular operating system system call for every implementation.

### 5.2 Preparing the CGI Request

Before executing the script, the server prepares the CGI request as described in Section 4 of RFC 3875.

The CGI request consists of two components:

1. Request meta-variables.
2. Request message data.

**Request meta-variables**

These contain descriptive information about the client request.

They must be immediately available to the script when execution begins.

Examples:

```text
REQUEST_METHOD=POST
SCRIPT_NAME=/cgi-bin/upload.py
QUERY_STRING=type=image
```

**Request message data**

This contains the data associated with the request message, such as its body.

Unlike the meta-variables, the request message data does not necessarily have to be immediately available to the script when execution begins.

### 5.3 Execution Before the Complete Request Body Is Received

RFC 3875 explicitly allows the server to execute the CGI script before it has received all request data from the client.

This means that receiving the complete HTTP body is not a mandatory prerequisite for starting CGI execution.

For example, consider the following request:

```http
POST /cgi-bin/upload.py HTTP/1.1
Host: example.com
Content-Type: application/octet-stream
Content-Length: 1000000
```

The client is transmitting a body containing 1,000,000 bytes.

The server can start the CGI program once the required request meta-variables are available, even if the complete body has not yet arrived.

Conceptually:

```text
CLIENT                  SERVER                   CGI
  |                        |                       |
  |---- HTTP Headers ----->|                       |
  |                        |                       |
  |                        |--- Prepare Variables->|
  |                        |                       |
  |                        |---- Execute CGI ----->|
  |                        |                       |
  |---- Body Part 1 ------>|---- Request Data ---->|
  |---- Body Part 2 ------>|---- Request Data ---->|
  |---- Body Part 3 ------>|---- Request Data ---->|
  |                        |                       |
  |---- End of Body ------>|---- End of Input ---->|
  |                        |                       |
```

This is a conceptual example of the possible execution order. The specification does not require every implementation to use this exact sequence or a particular communication mechanism.

The important point is that CGI execution can begin before all request data has been received.

---

## 6. Abnormal Script Termination

### 6.1 Server-Controlled Interruption

RFC 3875 establishes that, in the event of an error condition, the server can interrupt or terminate script execution at any time and without warning.

An example is a transport failure between the server and the client.

This means that a CGI program cannot assume that its execution will always finish normally.

### 6.2 Script Responsibility

The specification states that the script SHOULD be prepared to handle abnormal termination.

This is a recommendation directed at the CGI script.

It does not mean that the script can prevent the server from terminating its execution.

### 6.3 Implications for Webserv

Although RFC 3875 describes the script's responsibility, the Webserv implementation must also manage the lifecycle of the CGI process.

Relevant considerations include:

- Detecting when a CGI process terminates.
- Handling abnormal execution.
- Avoiding processes that remain running indefinitely.
- Managing process cancellation when necessary.
- Ensuring that process termination does not compromise the HTTP server.

These are architectural concerns that must be addressed when integrating CGI into the non-blocking server.

The specific process-management mechanisms will be studied separately.

---

## 7. Webserv Project Requirements

### 7.1 CGI Selection

The Webserv subject (v24.0) establishes that CGI execution must be based on the requested resource's file extension.

For example:

```text
script.py  -> Python CGI
script.php -> PHP CGI
```

The actual interpreter associated with each extension depends on the server configuration.

This is a project-specific requirement and must not be confused with a universal CGI rule.

RFC 3875 describes the general script-selection mechanism but does not require selection based exclusively on file extensions.

### 7.2 Process Execution

The Webserv subject permits the use of `fork()` for CGI execution.

This provides a concrete process-based execution model for our implementation.

The use of `fork()` is a project-level implementation constraint, not a general requirement of CGI/1.1.

### 7.3 Request Preparation

The Webserv subject requires the server to make the complete request and client-provided arguments available to the CGI program.

It also establishes specific requirements for request-body handling:

- The server must decode chunked request bodies before providing them to CGI.
- The CGI program must receive EOF as the indication that the request body has ended.

These requirements will influence the design of the communication mechanism between the server and the CGI process.

---

## 8. Key Takeaways

- The HTTP server acts as an application gateway between the client and the CGI program.
- The server remains responsible for HTTP communication even if the CGI script fails.
- Authentication and access controls must be satisfied before executing a protected CGI script.
- Script selection is based on the hierarchical URI path.
- The script path identifies the CGI program, while the remaining path identifies a resource or sub-resource interpreted by the script.
- `SCRIPT_NAME`, `PATH_INFO`, and `QUERY_STRING` describe the relevant URI components.
- The Script-URI is a reconstructible URI representing an equivalent CGI invocation.
- Script execution is system-defined.
- Request meta-variables must be immediately available when the script begins execution.
- The complete request message data does not have to be available when execution starts.
- The server may interrupt or terminate script execution in the event of an error.
- The CGI script SHOULD be prepared to handle abnormal termination.
- Webserv adds specific requirements concerning extension-based CGI selection, `fork()`, request-body handling, EOF, and non-blocking I/O.
