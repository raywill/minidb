#!/bin/bash

# Run all minitest tests
# Usage: ./run_all.sh [create|compare] [host] [port]

MODE="${1:-compare}"
HOST="${2:-127.0.0.1}"
PORT="${3:-9876}"

MINITEST="../../bin/minitest"
TEST_DIR="src"
REF_DIR="ref"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if minitest binary exists
if [ ! -f "$MINITEST" ]; then
    echo -e "${RED}Error: minitest binary not found at $MINITEST${NC}"
    echo "Please run 'make minitest' first"
    exit 1
fi

# Find all .sql files in src directory
SQL_FILES=$(find "$TEST_DIR" -name "*.sql" | sort)

if [ -z "$SQL_FILES" ]; then
    echo -e "${YELLOW}No test files found in $TEST_DIR${NC}"
    exit 0
fi

echo "======================================"
echo "  MiniDB Regression Test Suite"
echo "======================================"
echo "Mode: $MODE"
echo "Host: $HOST"
echo "Port: $PORT"
echo "======================================"
echo

TOTAL=0
PASSED=0
FAILED=0

for sql_file in $SQL_FILES; do
    TOTAL=$((TOTAL + 1))
    echo -n "Running $sql_file ... "

    if $MINITEST --run-mode=$MODE --host=$HOST --port=$PORT "$sql_file" > /dev/null 2>&1; then
        echo -e "${GREEN}PASS${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED=$((FAILED + 1))
        # Show detailed output on failure
        $MINITEST --run-mode=$MODE --host=$HOST --port=$PORT "$sql_file"
    fi
done

echo
echo "======================================"
echo "  Test Results"
echo "======================================"
echo "Total:  $TOTAL"
echo -e "Passed: ${GREEN}$PASSED${NC}"
if [ $FAILED -gt 0 ]; then
    echo -e "Failed: ${RED}$FAILED${NC}"
else
    echo -e "Failed: $FAILED"
fi
echo "======================================"

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
