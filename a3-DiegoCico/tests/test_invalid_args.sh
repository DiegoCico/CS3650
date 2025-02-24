#!/bin/bash
set -e

EXPECTED="Two arguments required."
OUTPUT=$($EXEC 2 2 2 2 2 2 2 2 2 || true)

if [[ "$OUTPUT" == "$EXPECTED" ]]; then
    echo "PASS: test_invalid_args"
else
    echo "FAIL: test_invalid_args"
    exit 1
fi
