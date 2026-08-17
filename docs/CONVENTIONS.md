# Code conventions

Conventions for the Webserver codebase. This project is subject to the
42 Norm and restricted to **C++98**. Follow these rules on every commit.

## 42 Norm

- **Indentation**: tabs, 4 space length.
- **Line length**: max 80 characters.
- **Functions**: keep them short and single-purpose; one statement per line.
- **No undefined behavior**, no leaks, no segfaults. All resources freed on
  exit.
- **Header includes**: order — standard library, then project headers.
- No forbidden functions. When in doubt, prefer plain C++98 + POSIX.

## Language constraints

- Strict **C++98**. 
- `std::string`, `std::vector`, `std::map` and the STL are allowed.
- No external libraries. Only the C++98 standard library and POSIX API.

## Forbidden C++11+ features

C++98 only. The features below (and any other C++11+ syntax) must **not** be
used; use the alternative instead:

| Forbidden                       | Alternative                    |
| ------------------------------- | ------------------------------ |
| `auto` / `decltype`             | explicit types                 |
| lambdas                         | functor classes                |
| range-based `for`               | iterator/index loops           |
| `nullptr`                       | `NULL` / `0`                   |
| `std::move`, rvalue refs `&&`   | plain by-reference passing     |
| smart pointers (`unique_ptr`...) | raw pointers + manual `delete` |
| `unordered_map` / `unordered_set` | `std::map` / `std::set`    |
| `std::array`, `std::tuple`      | C arrays / `std::pair` / `std::vector` |
| `std::to_string`, `stoi`, `stod` | `ostringstream`, `strtol`/`atoi` |
| `std::thread`, `std::mutex`, `std::atomic` | POSIX threads / `poll` (single-threaded) |
| `std::regex`, `std::chrono`, `std::random` | manual parsing, `gettimeofday`, POSIX |
| `std::function`, `std::bind`    | function pointers / functors  |
| `enum class`, `constexpr`, `static_assert`, `override`, `noexcept` | plain `enum`, `static const`, runtime asserts, comments |
| `initializer_list` (`v{1,2,3}`), uniform init | `push_back` / C arrays |


## Naming

| Item              | Convention                | Example                |
| ----------------- | ------------------------- | ---------------------- |
| Files             | `snake_case`              | `http_parser.cpp`      |
| Headers           | `snake_case.hpp`          | `http_parser.hpp`      |
| Classes           | `PascalCase`              | `HttpParser`           |
| Methods/functions | `camelCase`               | `parseRequestLine`     |
| Variables         | `snake_case`              | `server_fd`            |
| Constants/macros  | `UPPER_SNAKE_CASE`        | `MAX_HEADER_SIZE`      |
| Include guards    | `UPPER_SNAKE_CASE`        | `HTTP_PARSER_HPP`      |

- **Data members**: trailing underscore (`name_`), uniformly, for **all**
  data members regardless of access level. It marks "this belongs to
  `this`", saving `this->` and avoiding shadowing with locals/parameters:
  ```cpp
  class Server {
      int port_;
      std::string host_;
  public:
      Server(int port, const std::string &host) : port_(port), host_(host) {}
  };
  ```
- **Reserved identifiers — never use**: names containing `__`, names starting
  with `_` + uppercase (`_Name`), or leading `_` + lowercase at global/file
  scope (`int _x;`). Leading underscore inside a class is legal but we do not
  use it; trailing underscore is always safe.

## Code organization

Split by responsibility, one concern per module:

```
src/
├── config/      # config file parsing and validation
├── http/        # HTTP request/response parsing and generation
├── server/      # sockets, event loop, connection handling
└── cgi/         # CGI execution and env handling
```

Headers live in `inc/` mirroring the `src/` layout.

Rules:

- One class per file, named after the file.
- No logic in headers; declare only.
- Use `const` wherever possible.
- We handle expected errors (HTTP statuses, syscall failures) with return 
  values / status enums. **No exceptions**; `assert()` and logging cover
  debugging (see [Logging & assertions](#logging--assertions)).
- No magic numbers: define named constants in the relevant header.

## Git

- Commit early, commit often; each commit is one logical change.
- Use conventional commits with the responsibility (scope) in parentheses:
  `<type> ( <scope> ): <short summary>`
- Types:

  | Type       | Use                                  |
  | ---------- | ------------------------------------ |
  | `feat`     | new feature                          |
  | `fix`      | bug fix                              |
  | `refactor` | restructure without changing behavior|
  | `docs`     | documentation                        |
  | `test`     | tests                                |
  | `style`    | formatting/naming, no logic change   |
  | `chore`    | build, tooling, maintenance          |
  | `perf`     | optimization                         |

- Scopes (responsibilities of this project):

  | Scope      | Area                              |
  | ---------- | --------------------------------- |
  | `parse`    | request / config parsing          |
  | `http`     | protocol, request / response      |
  | `server`   | sockets, event loop, connections  |
  | `cgi`      | CGI execution                     |
  | `config`   | config file parsing / validation  |
  | `response` | response generation               |
  | `test`     | test suite                        |
  | `build`    | Makefile                          |
  | `docs`     | documentation                     |

- Examples:
  - `fix (parse): handle missing Host header`
  - `feat (server): add keep-alive support`
  - `refactor (http): split parser into small classes`
- Write summaries in English, imperative mood, first line ≤ 72 chars; add a
  body explaining the *why* when needed.
- Work on a branch per feature/milestone, merge to `main` when stable.
- Do not commit build artifacts or config files with secrets.

## Logging & assertions

- `assert()` is for **invariants and preconditions of our own code only**
  (e.g. `assert(state_ == READY)`). Never assert on external input: client
  data is handled with return codes, not by aborting.
- Log through the `Logger` class methods (`debug`/`info`/`warning`/`error`/
  `critical`, levels in `Logger::Level`); no bare `printf`/`cerr` in
  production code.
- `DEBUG` for internal trace, `INFO` for lifecycle and requests, `WARNING`/
  `ERROR`/`CRITICAL` for failures.
- Do **not** check `errno` after `read`/`write` (subject rule); log custom
  messages instead of `strerror(errno)`.
- Full design and current implementation in [LOGGING.md](LOGGING.md).

## Testing

- Every new module must come with tests in `tests/`.
- Existing behaviour must not regress; run the full suite before merging.
- See [TESTING.md](TESTING.md) for how to run and write tests.
