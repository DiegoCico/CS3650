#!/bin/bash
set -e

EXPECTED="beer"
OUTPUT=$($EXEC 10 -3)

if [[ "$OUTPUT" == "$EXPECTED" ]]; then
    echo "PASS: test_positive_result"
else
    echo "FAIL: test_positive_result"
    exit 1
fi
