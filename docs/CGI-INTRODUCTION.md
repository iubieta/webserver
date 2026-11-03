# CGI Introduction

## 1. Purpose of This Document

This document introduces the Common Gateway Interface (CGI), its purpose, fundamental concepts, terminology, and role in HTTP server architecture.

It is based primarily on **RFC 3875 — The Common Gateway Interface (CGI) Version 1.1**.

It also establishes the relationship between the general CGI specification and the requirements of the Webserv project.

The objective is to understand what CGI is and how it fits into an HTTP server before studying its request-processing rules, process execution, communication mechanisms, and implementation details.

---

## 2. CGI Overview

### 2.1 What Is CGI?

The Common Gateway Interface (CGI) is an interface that allows an HTTP server to invoke external software to process client requests.

It defines a common way for an HTTP server and a CGI program to exchange information, independently of the specific platform or operating system.

CGI establishes an interface between two components:

- **HTTP Server:** Receives client requests and manages HTTP communication.
- **CGI Program:** Processes application-specific operations and generates the corresponding output.

The server and the CGI program share responsibility for producing the response to a client request.

### 2.2 Separation of Responsibilities

CGI separates the responsibilities of the HTTP server and the CGI program.

Each component focuses on a different part of request processing.

**HTTP Server responsibilities:**

- Managing client connections.
- Receiving HTTP requests.
- Handling data transfer.
- Managing transport and network-related operations.
- Invoking the appropriate CGI program.
- Delivering the resulting response to the client.

**CGI Program responsibilities:**

- Processing application-specific requests.
- Accessing application data.
- Processing documents.
- Executing application logic.
- Producing output for the server.

This separation allows both components to operate through a common interface without requiring either one to understand the internal implementation of the other.

### 2.3 Advantages of This Separation

The separation of responsibilities provides several architectural advantages:

1. **Modularity:** The HTTP server and CGI programs can be developed and maintained independently.
2. **Reusability:** Different CGI programs can work with the same server interface.
3. **Maintainability:** Application logic can change without necessarily modifying the HTTP server.
4. **Extensibility:** New application capabilities can be introduced by adding CGI programs.
5. **Separation of concerns:** The server does not need to implement the internal logic of every application it supports.

---

## 3. Client Request and CGI Communication

### 3.1 Components of an HTTP Request

According to RFC 3875, a client request contains information that the CGI interface makes available to the invoked program.

The request includes:

- **URI:** Identifies the requested resource.
- **Request Method:** Indicates the operation requested by the client, such as `GET` or `POST`.
- **Ancillary Information:** Additional information provided through the HTTP protocol and related request context.

For example:

```http
GET /cgi-bin/report.py?year=2026 HTTP/1.1
Host: example.com
User-Agent: Mozilla/5.0
Accept: text/html
```

In this example:

| Request element | Description |
|---|---|
| `/cgi-bin/report.py` | Identifies the requested script resource. |
| `year=2026` | Query string associated with the URI. |
| `GET` | HTTP request method. |
| `HTTP/1.1` | HTTP protocol version. |
| `Host`, `User-Agent`, `Accept` | HTTP request headers. |

Not every HTTP header automatically becomes a CGI meta-variable. The CGI specification defines how relevant request information is represented and made available to the program.

### 3.2 CGI Meta-Variables

A **meta-variable** is a named parameter used to communicate information from the server to the CGI program.

Meta-variables describe information about the request or the server environment.

Examples include:

```text
REQUEST_METHOD=GET
QUERY_STRING=year=2026
SERVER_PROTOCOL=HTTP/1.1
```

These examples represent:

- `REQUEST_METHOD`: The HTTP method used by the client.
- `QUERY_STRING`: The query component of the requested URI.
- `SERVER_PROTOCOL`: The protocol version used for the request.

Meta-variables are an abstract concept defined by CGI. They describe the information that must be communicated, not necessarily the specific mechanism used to transport it.

### 3.3 Meta-Variables vs. Operating System Environment Variables

A CGI meta-variable is not necessarily an operating system environment variable.

The distinction is important:

| CGI Meta-Variable | Operating System Environment Variable |
|---|---|
| Defined by the CGI specification. | Provided through the operating system's process environment mechanism. |
| Represents information that the CGI interface makes available to the program. | Represents a named value available to a process. |
| Defines the meaning of the information. | Provides one possible mechanism for transporting that information. |

A common CGI implementation uses operating system environment variables to make meta-variables available to the CGI program.

However, the CGI specification defines the abstract interface, not an exclusive implementation mechanism.

This distinction becomes relevant when preparing the execution environment of a CGI process.

---

## 4. Normative Requirements

### 4.1 Requirement Keywords

RFC 3875 uses normative keywords to express the strength of its requirements.

Their interpretation follows the conventions established by RFC 2119 and BCP 14.

| Keyword | Meaning |
|---|---|
| `MUST` | An absolute requirement. |
| `MUST NOT` | An absolute prohibition. |
| `REQUIRED` | Equivalent to `MUST`. |
| `SHALL` | Equivalent to `MUST`. |
| `SHALL NOT` | Equivalent to `MUST NOT`. |
| `SHOULD` | A recommendation that normally ought to be followed. |
| `SHOULD NOT` | A recommendation against a particular behavior. |
| `RECOMMENDED` | Equivalent to `SHOULD`. |
| `MAY` | An optional feature or behavior. |
| `OPTIONAL` | Equivalent to `MAY`. |

A `MUST` requirement is mandatory for an implementation claiming conformance to the relevant specification.

A `SHOULD` recommendation is not necessarily mandatory, but deviations should only be made after understanding the consequences and having a valid reason.

### 4.2 Compliance Levels

RFC 3875 distinguishes between two compliance levels:

**Unconditional compliance**

An implementation satisfies all applicable `MUST` and `SHOULD` requirements for the features it implements.

**Conditional compliance**

An implementation satisfies all applicable `MUST` requirements but does not necessarily satisfy every `SHOULD` recommendation.

The distinction helps identify whether an implementation meets mandatory requirements and whether it also follows the specification's recommendations.

---

## 5. Specification-Defined Behavior

RFC 3875 distinguishes between system-defined and implementation-defined characteristics.

### 5.1 System-Defined

A characteristic is **system-defined** when its behavior may vary between different systems but must remain consistent among different implementations running on the same system.

The term *system* generally refers to a class of operating systems.

In other words, the behavior is determined by the underlying system rather than independently by each CGI implementation.

### 5.2 Implementation-Defined

A characteristic is **implementation-defined** when its behavior may vary between different implementations.

Each implementation must document the behavior it chooses.

This allows implementations to make certain design decisions while ensuring that their behavior is understandable and documented.

### 5.3 Why This Distinction Matters

When implementing CGI, it is important to distinguish between:

- Behavior specified by the CGI standard.
- Behavior determined by the underlying operating system.
- Behavior selected by the particular server or CGI implementation.

A design decision made by Webserv must not automatically be presented as a requirement of RFC 3875.

If a behavior is implementation-defined, the implementation is responsible for documenting it.

---

## 6. CGI Terminology

RFC 3875 defines several fundamental terms.

### 6.1 Meta-Variable

A named parameter that carries information from the server to the CGI program.

A meta-variable does not necessarily correspond to an operating system environment variable, although this is a common implementation approach.

### 6.2 Script

Software invoked by the server through the CGI interface to process a request.

The term is broader than a conventional script file. A CGI script can be:

- A standalone program.
- A dynamically loaded or shared library.
- A subroutine within the server.
- Instructions interpreted at runtime.

Therefore, CGI does not require the script to be written in a particular programming language or to have a specific file extension.

### 6.3 Server

The application that invokes a CGI script to handle client requests.

In the context of this project, the server is our Webserv HTTP server.

---

## 7. CGI Architecture Overview

At a conceptual level, CGI establishes an interaction between the HTTP server and the CGI program.

The following diagram illustrates the general relationship:

```text
                  HTTP CLIENT
                       |
                       | HTTP Request
                       v
                +--------------+
                |  HTTP SERVER |
                +--------------+
                       |
                       | CGI Interface
                       | Request Information
                       v
                +--------------+
                |  CGI PROGRAM |
                +--------------+
                       |
                       | Application Processing
                       |
                       | Generated Output
                       v
                +--------------+
                |  HTTP SERVER |
                +--------------+
                       |
                       | HTTP Response
                       v
                  HTTP CLIENT
```

The diagram represents the logical responsibilities and information flow. It does not prescribe a specific process architecture or operating system communication mechanism.

The CGI specification defines the interface between the server and the CGI program. The concrete execution and communication mechanisms depend on the implementation and the applicable system requirements.

---

## 8. CGI in the Webserv Project

### 8.1 Project Context

The Webserv project requires the implementation of an HTTP server capable of executing CGI programs.

The project subject establishes specific requirements that must be considered alongside the general CGI specification.

According to the Webserv subject (v24.0):

- CGI execution must be based on the requested resource's file extension.
- At least one CGI must be supported, such as PHP-CGI or Python.
- The server must investigate the environment variables used to communicate with CGI programs.
- The complete request and client-provided arguments must be made available to the CGI program.
- Chunked request bodies must be decoded by the server before being provided to CGI.
- The CGI program must receive the request body with EOF indicating its end.
- If the CGI output does not provide a `Content-Length`, EOF indicates the end of the output.
- CGI execution must take place in the appropriate working directory so that relative paths work correctly.

### 8.2 Non-Blocking Server Constraints

The Webserv subject also establishes constraints regarding I/O operations and process execution.

In particular:

- The server must not block indefinitely.
- `fork()` is permitted for CGI execution.

These constraints are especially important when designing CGI communication, because the CGI program and the HTTP server must exchange data without compromising the server's non-blocking architecture.

### 8.3 Scope of Our Implementation

The general CGI specification describes a broader interface than the specific execution model required by the Webserv subject.

For this project, our implementation will focus on executing external CGI programs.

The architecture, process lifecycle, data exchange, and integration with the existing HTTP server will be studied and documented in subsequent sections.

It is important to keep three categories separate throughout development:

1. **RFC requirements:** Behavior defined by the CGI specification.
2. **Webserv subject requirements:** Mandatory constraints established by the project.
3. **Implementation decisions:** Architectural and technical choices made by our team.

This distinction will help prevent implementation decisions from being confused with standard requirements.

---

## 9. Key Takeaways

- CGI defines an interface between an HTTP server and software invoked to process client requests.
- The server handles HTTP communication and transport, while the CGI program handles application-specific processing.
- CGI meta-variables are abstract named parameters that communicate request and server information.
- Operating system environment variables are a common mechanism for exposing meta-variables to CGI programs, but they are not the definition of meta-variables themselves.
- RFC 3875 uses normative keywords such as `MUST`, `SHOULD`, and `MAY` to express requirement strength.
- System-defined and implementation-defined characteristics identify where behavior is determined.
- The term `script` includes more than standalone Python or PHP files.