# Open Questions

Anything we do not understand yet, or any behaviour we have not
verified. A question leaves this file only when it is answered with
evidence, not with an opinion.

Move answered entries to the "Answered" section — do not delete them.
They are defence material.

Priority: `blocker` (stops work now) | `soon` | `later`

## Open

### Q-001 — What C functions and libraries are allowed?

- **Raised:** 2026-08-12 — **Answered:** YYYY-MM-DD
- **Answer:** 
- **Evidence:** Not sure if Ctime or time.h can be used or other c-style 
libraries or functions.
- **Consequence:** time.h is being used in Logger class. 

### Q-003 - How a duplicated interface:port server config should be treated?

- **Raised:** 2026-08-18 — **Answered:** YYYY-MM-DD
- **Answer:** 
- **Evidence:** bind con only be called once in a specific interface:port 
combination. If a server config file declare it twice the server must handle
it correctly
- **Consequence:** Implement deduplication in server core.

---

## Answered

### Q-002 — What is mutable and can it be used?

- **Raised:** 2026-08-12 — **Answered:** 2026-08-12
- **Answer:** Similar to const but in a logical sense. Yes can be used.
- **Evidence:** 
- **Consequence:** Mutable allows to call const function over a "non" physical
const object. It used to mark the object as logical const although what it 
points to or represents changes. 
- **Example:** an ofstream can be mutable because the file it represents doesnt
change but the file content can change.
