# Open Questions

Anything we do not understand yet, or any behaviour we have not
verified. A question leaves this file only when it is answered with
evidence, not with an opinion.

Move answered entries to the "Answered" section — do not delete them.
They are defence material.

Priority: `blocker` (stops work now) | `soon` | `later`

---

## Open

### Q-004 — <the question, phrased as a question>

- **Raised:** YYYY-MM-DD by <login>
- **Priority:** soon
- **Area:** sockets / http / config / cgi / build / other

**Why it matters:** what breaks or stays undecided until we know.

**What we already believe:** current guess, so we can check whether we
were wrong later.

**How we will find out:** RFC section, man page, NGINX experiment,
peer, staff.

---

### Q-003 — Example: does `poll` reporting POLLIN guarantee a whole
request is available?

- **Raised:** 2026-01-01 by <login>
- **Priority:** blocker
- **Area:** sockets

**Why it matters:** it decides whether the HTTP parser can be a
one-shot function or must be an incremental state machine.

**What we already believe:** no guarantee; readiness only means at
least one byte can be read without blocking.

**How we will find out:** `man 2 poll`, plus an experiment sending a
request one byte at a time with `nc`.

---

## Answered

### Q-001 — <the question>

- **Raised:** YYYY-MM-DD — **Answered:** YYYY-MM-DD
- **Answer:** the short version.
- **Evidence:** RFC 7230 section X.Y / `man 2 recv` / experiment in
  `docs/observations-http.md`.
- **Consequence:** links to the decision it produced, if any (D-XXX).
