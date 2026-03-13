#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_BINARY="${ROOT_DIR}/build-cpp-tests/IdpProtocol.UnitTests"

if [[ ! -x "${TEST_BINARY}" ]]; then
    echo "Test binary not found at ${TEST_BINARY}" >&2
    echo "Run ./build-tests.sh first." >&2
    exit 1
fi

if [[ $# -gt 0 ]]; then
    "${TEST_BINARY}" "$@"
    exit 0
fi

failures=0

while IFS= read -r test_name; do
    [[ -z "${test_name}" ]] && continue

    echo "Running: ${test_name}"

    if ! "${TEST_BINARY}" "${test_name}"; then
        failures=$((failures + 1))
    fi
done < <("${TEST_BINARY}" --list-test-names-only)

if [[ ${failures} -ne 0 ]]; then
    echo
    echo "${failures} test case(s) failed." >&2
    exit 1
fi

echo
echo "All test cases passed."
