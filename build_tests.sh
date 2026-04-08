#!/bin/bash
set -euo pipefail

flatc --cpp -o include/afware/rsp ./schema/scope_info.fbs

mkdir -p bin

CXXFLAGS="-std=c++23 -Wall -Wextra -pedantic -Iinclude/ -DRSP_ENABLE"
LDFLAGS="-lgtest -lgtest_main -lpthread"

ENABLED_TESTS=(
    tests/profiler_test_env.cpp
    tests/test_constexpr_string.cpp
    tests/test_metadata.cpp
    tests/test_slots.cpp
    tests/test_scope_info.cpp
    tests/test_scope_manager.cpp
    tests/test_machine.cpp
    tests/test_serialization.cpp
    tests/test_sinks.cpp
    tests/test_profiler.cpp
    tests/test_active_scope.cpp
    tests/test_api_enabled.cpp
    tests/test_integration.cpp
    tests/test_threading.cpp
)

echo "Building main test binary..."
clang++ $CXXFLAGS "${ENABLED_TESTS[@]}" $LDFLAGS -o bin/rsp_tests

echo "Building disabled-API test binary..."
clang++ -std=c++23 -Wall -Wextra -pedantic -Iinclude/ \
    tests/test_api_disabled.cpp \
    -lgtest -lgtest_main -lpthread \
    -o bin/rsp_tests_disabled

echo ""
echo "=== Running main test suite ==="
./bin/rsp_tests "$@"

echo ""
echo "=== Running disabled-API tests ==="
./bin/rsp_tests_disabled "$@"
