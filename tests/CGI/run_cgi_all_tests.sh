#!/bin/sh
# Run the available standalone CGI integration test.
set -eu

cd "$(dirname "$0")/../.."

build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT HUP INT TERM

sources=$(find src -name '*.cpp' ! -name main.cpp)

${CXX:-c++} -std=c++98 -Wall -Wextra -Werror \
    ${CGI_DEBUG_FLAGS:-} \
    -Iinc \
    tests/CGI/cgi_manager_event_loop_test.cpp \
    $sources \
    -o "$build/harness"

"$build/harness"

printf '%s\n' 'CGI event-loop test passed'
