#!/bin/bash
#
# TBOL Compiler Test Runner
#
# Positive tests: compile with tbolc, compare bytecode against reference .cod
# Negative tests: compile with tbolc, verify non-zero exit and expected error message
#
# Usage:
#   ./run_tests.sh                    # Run all tests
#   ./run_tests.sh positive/verbs     # Run one category
#   ./run_tests.sh positive/verbs/add_basic.src  # Run one test
#
# Environment:
#   TBOLC       - path to tbolc (default: ../../build/compiler/tbolc)
#   GETBYTECODE - path to getbytecode (default: ../../tools/getbytecode)
#   UPDATE      - set to 1 to update reference files from tbolc output

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TBOLC="${TBOLC:-$SCRIPT_DIR/../../build/compiler/tbolc}"
GETBYTECODE="${GETBYTECODE:-$SCRIPT_DIR/getbytecode}"

# Build getbytecode if needed
if [ ! -x "$GETBYTECODE" ]; then
    gcc -Wall -Wextra -std=c11 -O2 -o "$GETBYTECODE" "$SCRIPT_DIR/getbytecode.c" 2>/dev/null
fi

# Colors
RED='\033[1;31m'
GREEN='\033[1;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass=0
fail=0
skip=0

run_positive_test() {
    local src="$1"
    local dir="$(dirname "$src")"
    local base="$(basename "$src" .src)"
    base="$(echo "$base" | tr '[:upper:]' '[:lower:]')"
    local reference="$dir/reference/${base}.cod"

    # Determine include paths (add the test's directory and preproc includes if present)
    local include_args=()
    if [ -d "$dir/includes" ]; then
        include_args+=(-I "$dir/includes")
    fi

    # Compile
    local workdir
    workdir="$(mktemp -d)"
    if ! "$TBOLC" ${include_args[@]+"${include_args[@]}"} -o "$workdir" "$src" >/dev/null 2>"$workdir/stderr"; then
        echo -e "  ${RED}FAIL${NC}: $src (compilation failed)"
        cat "$workdir/stderr" | head -5 | sed 's/^/        /'
        rm -rf "$workdir"
        ((fail++))
        return
    fi

    # Find the output .cod file
    local cod
    cod="$(ls "$workdir"/*.cod 2>/dev/null | head -1)"
    if [ -z "$cod" ]; then
        echo -e "  ${RED}FAIL${NC}: $src (no .cod produced)"
        rm -rf "$workdir"
        ((fail++))
        return
    fi

    # If UPDATE mode, copy output as reference
    if [ "${UPDATE:-0}" = "1" ]; then
        mkdir -p "$dir/reference"
        cp "$cod" "$reference"
        echo -e "  ${YELLOW}UPDATED${NC}: $src"
        rm -rf "$workdir"
        ((pass++))
        return
    fi

    # Compare against reference
    if [ ! -f "$reference" ]; then
        echo -e "  ${YELLOW}SKIP${NC}: $src (no reference file)"
        rm -rf "$workdir"
        ((skip++))
        return
    fi

    if diff <("$GETBYTECODE" "$cod") <("$GETBYTECODE" "$reference") >/dev/null 2>&1; then
        echo -e "  ${GREEN}PASS${NC}: $src"
        ((pass++))
    else
        echo -e "  ${RED}FAIL${NC}: $src (bytecode differs from reference)"
        echo "        tbolc output: $(xxd "$cod" | head -3)"
        echo "        reference:       $(xxd "$reference" | head -3)"
        ((fail++))
    fi

    rm -rf "$workdir"
}

run_negative_test() {
    local src="$1"
    local dir="$(dirname "$src")"
    local base="$(basename "$src" .src)"
    local expected="$dir/${base}.expected"

    # Compile — should fail
    local workdir
    workdir="$(mktemp -d)"
    local stderr_file="$workdir/stderr"

    if "$TBOLC" -o "$workdir" "$src" >/dev/null 2>"$stderr_file"; then
        echo -e "  ${RED}FAIL${NC}: $src (expected error but compilation succeeded)"
        rm -rf "$workdir"
        ((fail++))
        return
    fi

    # Check expected error pattern if .expected file exists
    if [ -f "$expected" ]; then
        local pattern
        pattern="$(cat "$expected")"
        if grep -qiE "$pattern" "$stderr_file"; then
            echo -e "  ${GREEN}PASS${NC}: $src"
            ((pass++))
        else
            echo -e "  ${RED}FAIL${NC}: $src (error output doesn't match expected pattern)"
            echo "        expected: $pattern"
            echo "        got:      $(head -1 "$stderr_file")"
            ((fail++))
        fi
    else
        # No expected file — just verify it failed
        echo -e "  ${GREEN}PASS${NC}: $src (correctly rejected)"
        ((pass++))
    fi

    rm -rf "$workdir"
}

# Determine what to run
target="${1:-all}"

collect_tests() {
    local dir="$1"
    local type="$2"  # positive or negative
    find "$dir" -name '*.src' -o -name '*.SRC' | sort
}

echo "TBOL Compiler Test Suite"
echo "========================"
echo ""

if [ "$target" = "all" ]; then
    # Run all positive tests
    if [ -d "$SCRIPT_DIR/positive" ]; then
        echo "Positive tests:"
        while IFS= read -r src; do
            run_positive_test "$src"
        done < <(collect_tests "$SCRIPT_DIR/positive" positive)
        echo ""
    fi

    # Run all negative tests
    if [ -d "$SCRIPT_DIR/negative" ]; then
        echo "Negative tests:"
        while IFS= read -r src; do
            run_negative_test "$src"
        done < <(collect_tests "$SCRIPT_DIR/negative" negative)
        echo ""
    fi
elif [ -f "$target" ]; then
    # Single file
    if [[ "$target" == *negative* ]]; then
        run_negative_test "$target"
    else
        run_positive_test "$target"
    fi
elif [ -d "$SCRIPT_DIR/$target" ]; then
    # Directory
    if [[ "$target" == negative* ]]; then
        echo "Negative tests ($target):"
        while IFS= read -r src; do
            run_negative_test "$src"
        done < <(collect_tests "$SCRIPT_DIR/$target" negative)
    else
        echo "Positive tests ($target):"
        while IFS= read -r src; do
            run_positive_test "$src"
        done < <(collect_tests "$SCRIPT_DIR/$target" positive)
    fi
fi

echo "========================"
echo -e "Results: ${GREEN}${pass} passed${NC}, ${RED}${fail} failed${NC}, ${YELLOW}${skip} skipped${NC}"

if [ "$fail" -gt 0 ]; then
    exit 1
fi
