#!/bin/bash
set -e

EXPECTED="tea"
OUTPUT=$($EXEC 2 2)

if [[ "$OUTPUT" == "$EXPECTED" ]]; then
    echo "PASS: test_valid_args"
else
    echo "FAIL: test_valid_args"
    exit 1
fi
