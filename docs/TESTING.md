# Testing

Guide for **running** and **creating** the Webserver tests. This document
describes the **current** test infrastructure.

Two layers are planned:

- **Unit tests** (C++) — parsers and pure logic, no network, no server.
- **Integration tests** (bash + `curl`/`nc`) — against the real server.

Only the **unit** layer exists today, with an in-house harness. No external
test framework (e.g. Google Test) is allowed.

## Current test files

```
tests/
├── Tester.hpp        # harness: ASSERT_EQ macro + test runner API
├── Tester.cpp        # harness implementation (counters, reporting)
├── main.cpp          # test registry: runs the selected test groups
├── TestList.hpp      # declarations of each test group entry point
├── loggerTest.cpp    # unit tests for the Logger class (group "logger")
└── run_tests         # compiled test binary (build artifact, in repo root)
```

`loggerTest.cpp` is the first unit test module. As new modules land, each
gets its own test file in `tests/` (e.g. `configParserTest.cpp`) plus a
group entry.

## Building and running

The [Makefile](../Makefile) builds both the server and the tests:

```sh
make              # build the server binary ./webserv
make test         # build the test binary ./run_tests and run it
make test ARGS=logger   # run only the "logger" test group
make clean        # remove obj/
make fclean       # remove obj/, ./webserv and ./run_tests
make re           # fclean + all
```

`make test` compiles `tests/Tester.cpp`, `tests/main.cpp`, `tests/*Test.cpp`
plus the project sources (`src/`), links `./run_tests` and executes it.
Passing `ARGS=<group>` runs only the matching group(s).

Output is colorized (green `OK`, red `KO`).

> **Note:** `run_tests` currently always exits with status 0, even if some
> tests fail — the failures are reported by the harness but **not**
> propagated to the exit code. Propagating `Tester::report()`'s value through
> `main.cpp` is a pending improvement.

## Test groups

Each test module exposes a single entry point, e.g.
`void logger_tests()`, that runs all the module's batteries. The registry
in `tests/main.cpp` lists the available groups:

```cpp
static const TestEntry g_tests[] = {
    { "logger", logger_tests },
};
```

Running `./run_tests` with no arguments runs every group in order; passing
group names as arguments filters to those:

```sh
./run_tests logger        # just the logger group
```

## Harness API

All tests share the tiny harness in `Tester.hpp` / `Tester.cpp`:

- `ASSERT_EQ(actual, expected)` — macro. If `(actual) != (expected)`, it
  prints a red failure line (file, line, expected vs. received), records a
  failure via `Tester::recordFail()`, and **returns from the current test
  function immediately**.
- `Tester::runTest(name, fn)` — runs one test function `fn()` and reports
  `name --> OK` / `name --> KO`.
- `Tester::report()` — prints the summary (`OK! All tests passed` or
  `FAIL: n/total`) and returns the number of failures (not used by the
  runner yet, see note above).

## How to write a new test module

For a module `Foo`, in `tests/fooTest.cpp`:

1. Write one `void` function per behaviour, following the naming
   `test<Something>`.
2. Use `ASSERT_EQ(actual, expected)` to assert (it returns early on failure,
   so assert once per concern or split into separate functions).
3. Group the batteries in one entry point `void foo_tests()` that calls
   `Tester::runTest("Name", testSomething);` for each and finishes with
   `Tester::report();`.
4. Declare the entry point in `tests/TestList.hpp`:

   ```cpp
   void foo_tests();
   ```

5. Register the group in `tests/main.cpp`:

   ```cpp
   { "foo", foo_tests },
   ```

6. Add the source to `TEST_SRCS` in the Makefile:

   ```make
   TEST_SRCS := Tester.cpp main.cpp \
                loggerTest.cpp \
                fooTest.cpp
   ```

Skeleton:

```cpp
// tests/fooTest.cpp
void testSomething() {
    int result = doTheThing();
    ASSERT_EQ(result, 42);
}

void foo_tests() {
    Tester::runTest("Something", testSomething);
    Tester::report();
}
```

## Current test batteries

`tests/loggerTest.cpp` covers the `Logger` class in seven batteries:

| #  | Battery              | What it checks                                        |
| -- | -------------------- | ----------------------------------------------------- |
| 1  | File error           | Opening an unwritable path → `[WARNING]` on console, continues in console mode |
| 2  | File Level filtering | Per-level file thresholds (`DEBUG`→5 … `CRITICAL`→1 lines) |
| 3  | Console Level filtering | Per-level console thresholds, messages counted on redirected `stderr` |
| 4  | Level independence   | File and console levels set independently, both honored |
| 5  | Console off          | `setConsole(0)` silences the console output           |
| 6  | Append mode          | Reopening the logger appends (5 → 10 lines), never truncates |
| 7  | Message fidelity     | Exact file line `[INFO] msg - file:line` and console line without `file:line` |

Helpers used by the tests: `logEveryLevel`, `numberOfLines`,
`redirCerr`/`resetCerr` (redirect `std::cerr` to an `ostringstream`),
`countCharInStr`, `fileHasLine`.

## Integration tests

> **Estado: pendiente.** Integration scenarios (bash + `curl`/`nc`) are
> planned but not implemented yet. Expected structure:

```
tests/
├── integration/         # shell scenarios, numbered in order
│   ├── 01_get.sh
│   ├── 02_post.sh
│   └── ...
└── fixtures/            # inputs shared by the tests
    ├── config/          #   sample server configs
    ├── files/           #   static files to serve
    └── cgi/             #   scripts for CGI scenarios
```

A scenario follows: **fixture → start server → request → assert → teardown**
(`set -euo pipefail`, start `./webserv` in background, `trap 'kill $PID'
EXIT`, assert with `curl`/`nc`).

## Quality gates

Before merging to `main`:

1. `make re` then `make test` — all tests pass.
2. Existing behaviour must not regress; run the full suite before merging.
3. New module ⇒ matching unit test in `tests/`.
