# Decision Log

Every architectural or design decision goes here, newest first.
One entry per decision. If you cannot fill the "Alternatives" field,
you have not decided anything yet — you have only picked the first
idea that came to mind.

Status values: `proposed` | `accepted` | `superseded by D-XX`

---

## D-002 — <short title>

- **Date:** YYYY-MM-DD
- **Status:** proposed
- **Owner:** <login>

### Context

What problem forced this decision? What constraint from the subject
or from the code triggered it?

### Decision

What we do. One or two sentences, in the imperative.

### Alternatives considered

| Option | Why rejected |
|---|---|
| A | |
| B | |

### Consequences

What this makes easy, what this makes hard, and what code depends on
it. Note here anything that would need rewriting if we reverse it.

### Defence answer

Two or three sentences answering "why did you do it this way?" as you
would say it out loud to an evaluator.

---

## D-001 — Example: connection objects are not copyable

- **Date:** 2026-01-01
- **Status:** accepted
- **Owner:** <login>

### Context

C++98 has no smart pointers. A connection owns a file descriptor.
Copying such an object risks closing the same fd twice.

### Decision

Copy constructor and `operator=` are declared private and left
unimplemented on every class that owns a system resource.

### Alternatives considered

| Option | Why rejected |
|---|---|
| Reference counting by hand | Complexity not justified at this scale |
| Store objects by value in containers | Containers copy on reallocation |

### Consequences

Such objects must live behind pointers in containers. Ownership rules
must be written down (see D-XXX).

### Defence answer

In C++98 there is no `= delete`, so the idiom to forbid copying is a
private undefined copy constructor. We forbid it because these classes
own a file descriptor and a double close is a silent, hard-to-trace
bug in a server.
