#!/bin/bash
#
# TBOL Formatter Test Runner
#
# Formats each input file with tbolfmt and compares against reference output.
#
# Usage:
#   ./run_tests.sh                  # Run all tests
#
# Environment:
#   TBOLFMT - path to tbolfmt (default: ../../build/formatter/tbolfmt)
#   UPDATE  - set to 1 to update reference files from tbolfmt output
#
# Each test runs twice and asserts byte-identical output - any pass that
# is not idempotent will fail here.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TBOLFMT="${TBOLFMT:-$SCRIPT_DIR/../../build/formatter/tbolfmt}"

# Colors
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass=0
fail=0

echo "TBOL Formatter Test Suite"
echo "========================="
echo ""

for input in "$SCRIPT_DIR"/input/*.src; do
    [ -f "$input" ] || continue
    base="$(basename "$input" .src)"
    reference="$SCRIPT_DIR/reference/${base}.src"

    actual="$("$TBOLFMT" "$input" 2>&1)"
    rc=$?

    if [ $rc -ne 0 ]; then
        echo -e "  ${RED}FAIL${NC}: $base (tbolfmt exited $rc)"
        echo "$actual" | head -3 | sed 's/^/        /'
        ((fail++))
        continue
    fi

    if [ "${UPDATE:-0}" = "1" ]; then
        echo "$actual" > "$reference"
        echo -e "  ${YELLOW}UPDATED${NC}: $base"
        ((pass++))
        continue
    fi

    if [ ! -f "$reference" ]; then
        echo -e "  ${YELLOW}SKIP${NC}: $base (no reference file)"
        continue
    fi

    if ! diff <(echo "$actual") "$reference" >/dev/null 2>&1; then
        echo -e "  ${RED}FAIL${NC}: $base (output != reference)"
        diff <(echo "$actual") "$reference" | head -10 | sed 's/^/        /'
        ((fail++))
        continue
    fi

    # Idempotency: running the formatter on its own output must
    # produce the same bytes.
    twice="$(echo "$actual" | "$TBOLFMT")"
    if ! diff <(echo "$actual") <(echo "$twice") >/dev/null 2>&1; then
        echo -e "  ${RED}FAIL${NC}: $base (not idempotent)"
        diff <(echo "$actual") <(echo "$twice") | head -10 | sed 's/^/        /'
        ((fail++))
        continue
    fi

    echo -e "  ${GREEN}PASS${NC}: $base"
    ((pass++))
done

echo ""
echo "========================="
echo -e "Results: ${GREEN}${pass} passed${NC}, ${RED}${fail} failed${NC}"

if [ "$fail" -gt 0 ]; then
    exit 1
fi
