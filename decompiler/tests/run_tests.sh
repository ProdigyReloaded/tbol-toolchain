#!/bin/bash
#
# TBOL Decompiler Test Runner
#
# Decompiles reference .cod files from the compiler test suite and verifies
# round-trip: decompile → recompile → identical bytecode.  The decompiler
# performs this verification internally, so a non-zero exit means the
# round-trip failed.
#
# Usage:
#   ./run_tests.sh                          # Run all tests
#   ./run_tests.sh encoding                 # Run one category
#   ./run_tests.sh verbs/add_basic          # Run one test
#
# Environment:
#   TBOLDC   - path to tboldc (default: ../tboldc)

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TBOLDC="${TBOLDC:-$SCRIPT_DIR/../tboldc}"
COMPILER_TESTS="$SCRIPT_DIR/../../compiler/tests/positive"

# Colors
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass=0
fail=0
skip=0

run_test() {
    local cod="$1"
    local category="$2"
    local base="$3"

    # Decompile with verification (default mode)
    local workdir
    workdir="$(mktemp -d)"
    local outfile="$workdir/${base}.src"

    if "$TBOLDC" -o "$outfile" "$cod" 2>"$workdir/stderr"; then
        # Exit 0 means round-trip verified
        if [ -s "$outfile" ]; then
            echo -e "  ${GREEN}PASS${NC}: $category/$base"
            ((pass++))
        else
            echo -e "  ${YELLOW}SKIP${NC}: $category/$base (empty output)"
            ((skip++))
        fi
    else
        echo -e "  ${RED}FAIL${NC}: $category/$base"
        head -3 "$workdir/stderr" | sed 's/^/        /'
        ((fail++))
    fi

    rm -rf "$workdir"
}

# Determine what to run
target="${1:-all}"

echo "TBOL Decompiler Test Suite"
echo "=========================="
echo ""

if [ "$target" = "all" ]; then
    for category_dir in "$COMPILER_TESTS"/*/; do
        category="$(basename "$category_dir")"
        [ "$category" = "includes" ] && continue
        reference_subdir="$category_dir/reference"
        [ -d "$reference_subdir" ] || continue
        for cod in "$reference_subdir"/*.cod; do
            [ -f "$cod" ] || continue
            base="$(basename "$cod" .cod)"
            run_test "$cod" "$category" "$base"
        done
    done
elif [ -d "$COMPILER_TESTS/$target" ]; then
    reference_subdir="$COMPILER_TESTS/$target/reference"
    if [ -d "$reference_subdir" ]; then
        for cod in "$reference_subdir"/*.cod; do
            [ -f "$cod" ] || continue
            base="$(basename "$cod" .cod)"
            run_test "$cod" "$target" "$base"
        done
    fi
else
    # Single test: target is "category/basename"
    category="$(dirname "$target")"
    base="$(basename "$target")"
    cod="$COMPILER_TESTS/$category/reference/${base}.cod"
    if [ -f "$cod" ]; then
        run_test "$cod" "$category" "$base"
    else
        echo -e "  ${RED}ERROR${NC}: $cod not found"
        ((fail++))
    fi
fi

echo ""
echo "=========================="
echo -e "Results: ${GREEN}${pass} passed${NC}, ${RED}${fail} failed${NC}, ${YELLOW}${skip} skipped${NC}"

if [ "$fail" -gt 0 ]; then
    exit 1
fi
