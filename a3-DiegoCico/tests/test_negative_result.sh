#!/bin/bash
set -e

EXPECTED="hat"
OUTPUT=$($EXEC -5 3)

if [[ "$OUTPUT" == "$EXPECTED" ]]; then
    echo "PASS: test_negative_result"
else
    echo "FAIL: test_negative_result"
    exit 1
fi
