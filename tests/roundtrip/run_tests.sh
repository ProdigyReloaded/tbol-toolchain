#!/bin/bash
#
# Round-trip test suite for TBOL compiler/decompiler
# Tests: compile -> decompile -> recompile = identical bytecode
#

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TBOLC="$SCRIPT_DIR/../../compiler/tbolc"
TBOLDC="$SCRIPT_DIR/../../decompiler/tboldc"
GETBYTECODE="$SCRIPT_DIR/../../tools/getbytecode"
INCLUDE_PATH="$SCRIPT_DIR/../orig/src"
RESULTS="$SCRIPT_DIR/results"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Check tools exist
if [ ! -x "$TBOLC" ]; then
    echo "Error: tbolc not found at $TBOLC"
    exit 1
fi

if [ ! -x "$TBOLDC" ]; then
    echo "Error: tboldc not found at $TBOLDC"
    exit 1
fi

if [ ! -x "$GETBYTECODE" ]; then
    echo "Error: getbytecode not found at $GETBYTECODE"
    echo "Run 'make' in tools/ directory first"
    exit 1
fi

# Create results directory
mkdir -p "$RESULTS"

# Counters
pass=0
fail_compile=0
fail_decompile=0
fail_recompile=0
fail_compare=0

# Process each .src file
for src in "$SCRIPT_DIR"/*.src; do
    [ -f "$src" ] || continue

    base=$(basename "$src" .src)

    # Step 1: Compile original source
    if ! timeout 5 "$TBOLC" -I "$INCLUDE_PATH" "$src" -o "$RESULTS" 2>"$RESULTS/$base.err1"; then
        echo -e "${RED}FAIL${NC}: $base (compile failed)"
        ((fail_compile++))
        continue
    fi

    # Extract bytecode from original
    "$GETBYTECODE" "$RESULTS/$base.cod" > "$RESULTS/$base.bc1"

    # Step 2: Decompile the .cod file
    if ! timeout 5 "$TBOLDC" -I "$INCLUDE_PATH" "$RESULTS/$base.cod" > "$RESULTS/$base.src2" 2>"$RESULTS/$base.err2"; then
        echo -e "${YELLOW}FAIL${NC}: $base (decompile failed)"
        ((fail_decompile++))
        continue
    fi

    # Step 3: Recompile decompiled source
    if ! timeout 5 "$TBOLC" -I "$INCLUDE_PATH" "$RESULTS/$base.src2" -o "$RESULTS" 2>"$RESULTS/$base.err3"; then
        echo -e "${YELLOW}FAIL${NC}: $base (recompile failed)"
        ((fail_recompile++))
        continue
    fi

    # Extract bytecode from recompiled
    "$GETBYTECODE" "$RESULTS/$base.cod" > "$RESULTS/$base.bc2"

    # Step 4: Compare bytecode
    if cmp -s "$RESULTS/$base.bc1" "$RESULTS/$base.bc2"; then
        echo -e "${GREEN}PASS${NC}: $base"
        ((pass++))
    else
        echo -e "${RED}FAIL${NC}: $base (bytecode differs)"
        size1=$(wc -c < "$RESULTS/$base.bc1")
        size2=$(wc -c < "$RESULTS/$base.bc2")
        echo "  Original: $size1 bytes, Recompiled: $size2 bytes"
        ((fail_compare++))
    fi
done

echo ""
echo "========================================"
total=$((pass + fail_compile + fail_decompile + fail_recompile + fail_compare))
echo "Results: $total tests"
echo -e "  ${GREEN}Pass:${NC} $pass"
echo -e "  ${RED}Compile fail:${NC} $fail_compile"
echo -e "  ${YELLOW}Decompile fail:${NC} $fail_decompile"
echo -e "  ${YELLOW}Recompile fail:${NC} $fail_recompile"
echo -e "  ${RED}Bytecode diff:${NC} $fail_compare"
echo "========================================"

if [ $fail_compile -gt 0 ] || [ $fail_compare -gt 0 ]; then
    exit 1
fi
exit 0
