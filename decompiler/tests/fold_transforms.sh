#!/bin/bash
#
# fold_transforms.sh - Regression test for readability fold transforms.
#
# The decompiler folds two compiler control-flow idioms back into their
# source-level shape:
#
#   1. OR guards - `IF a OR b OR c THEN ...` and `WHILE a OR b THEN ...`,
#      which tbolc compiles to a chain of inverted CJs routing to a shared
#      body. Without folding these decompile to a ladder of `IF .. THEN
#      GOTO` / `DO` blocks.
#   2. Else-if ladders - `IF .. ELSE IF .. ELSE`, which the decompiler used
#      to emit as right-nested `ELSE DO { IF .. } END` pyramids.
#
# Both folds are bytecode-neutral: the fixtures are compiler golden .cod
# files that decompile WITHOUT -f (so a round-trip is verified as a side
# effect - non-empty output means recompilation matched the original), and
# we additionally assert the folded source shape is present.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TBOLDC="${TBOLDC:-$SCRIPT_DIR/../tboldc}"
REF="$SCRIPT_DIR/../../compiler/tests/positive/control/reference"
EDGE="$SCRIPT_DIR/../../compiler/tests/positive/edge/reference"

RED='\033[1;31m'; GREEN='\033[1;32m'; NC='\033[0m'
fail=0

# decompile <cod> -> writes $out, verifying round-trip (no -f).
out="$(mktemp)"

# check <label> <cod> <present-regex> <absent-regex-or-empty>
check() {
    local label="$1" cod="$2" want="$3" avoid="$4"
    if [ ! -f "$cod" ]; then
        echo -e "  ${RED}FAIL${NC}: $label (fixture missing: $cod)"; fail=1; return
    fi
    : > "$out"
    "$TBOLDC" -o "$out" "$cod" 2>/dev/null
    if [ ! -s "$out" ]; then
        echo -e "  ${RED}FAIL${NC}: $label (no output - round-trip failed)"; fail=1; return
    fi
    if ! grep -qE "$want" "$out"; then
        echo -e "  ${RED}FAIL${NC}: $label (expected /$want/, not found)"; fail=1; return
    fi
    if [ -n "$avoid" ] && grep -qE "$avoid" "$out"; then
        echo -e "  ${RED}FAIL${NC}: $label (unwanted /$avoid/ present)"; fail=1; return
    fi
    echo -e "  ${GREEN}PASS${NC}: $label"
}

# count <label> <cod> <regex> <min-count>
count() {
    local label="$1" cod="$2" re="$3" min="$4"
    if [ ! -f "$cod" ]; then
        echo -e "  ${RED}FAIL${NC}: $label (fixture missing: $cod)"; fail=1; return
    fi
    : > "$out"
    "$TBOLDC" -o "$out" "$cod" 2>/dev/null
    local n
    n="$(grep -cE "$re" "$out" 2>/dev/null || echo 0)"
    if [ ! -s "$out" ]; then
        echo -e "  ${RED}FAIL${NC}: $label (no output - round-trip failed)"; fail=1; return
    fi
    if [ "$n" -lt "$min" ]; then
        echo -e "  ${RED}FAIL${NC}: $label (expected >=$min /$re/, got $n)"; fail=1; return
    fi
    echo -e "  ${GREEN}PASS${NC}: $label ($n)"
}

# --- OR guard folds ---
check "2-operand OR guard folds"       "$REF/bool_or_simple.cod" \
      'IF \(.* OR .*\) THEN' 'THEN GOTO'
check "3-operand OR cascade folds"     "$REF/bool_if_or_eq.cod" \
      'IF \(.* OR .* OR .*\) THEN' 'THEN GOTO'
check "OR guard folds under WHILE"     "$REF/while_or.cod" \
      'WHILE \(.* OR .*\) THEN' 'THEN GOTO'

# --- Else-if ladder flatten ---
count "else-if ladder flattens (chain)" "$REF/if_else_if_chain.cod" \
      '^[[:space:]]*ELSE IF ' 2
check "else-if ladder has no nesting"   "$REF/if_else_if_chain.cod" \
      '^[[:space:]]*ELSE IF ' 'ELSE DO'
count "long else-if ladder flattens"    "$REF/if_else_chain_5.cod" \
      '^[[:space:]]*ELSE IF ' 3

rm -f "$out"
exit $fail
