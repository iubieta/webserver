This project has been created as part of the 42 curriculum by iubieta- and
ypacileo.

# Webserver

HTTP/1.1 web server written in C++98, built as part of the 42 curriculum.
This is a non-blocking server that parses a configuration file and serves
static files and CGI scripts.

## Authors

- iubieta-
- ypacileo

## Repository layout

```
├── config/   # example configuration files
├── docs/     # project documentation (see table of contents below)
├── inc/      # headers (.hpp)
├── res/      # static resources served as error pages / assets
├── src/      # sources (.cpp)
├── tests/    # test suite and fixtures
├── www/      # default web root (static content + cgi-bin)
├── Makefile
└── README.md
```

## Requirements

- C++98 compiler (`c++` with `-std=c++98`)
- GNU `make`
- Linux (poll-based multiplexing)

## Build

```sh
make          # compile the project
make clean    # remove object files
make re       # clean + rebuild
make fclean   # remove objects, binary and dependencies
```

## Run

```sh
./webserv [config_file]
```

If no config file is given, a default configuration is used (see
`config/`). See [docs/CHEATSHEET.md](docs/CHEATSHEET.md) for usage examples.

## Documentation

| File                         | Contents                                  |
| ---------------------------- | ----------------------------------------- |
| [docs/ROADMAP.md](docs/ROADMAP.md)           | Project milestones and progress   |
| [docs/CONVENTIONS.md](docs/CONVENTIONS.md)   | Code style and git conventions     |
| [docs/LOGGING.md](docs/LOGGING.md)           | Logging module design              |
| [docs/TESTING.md](docs/TESTING.md)           | How to run and write tests         |
| [docs/CHEATSHEET.md](docs/CHEATSHEET.md)     | Build/run/test usage cheatsheet    |
| [docs/HTTP-REFERENCE.md](docs/HTTP-REFERENCE.md) | HTTP/1.1 quick reference        |
