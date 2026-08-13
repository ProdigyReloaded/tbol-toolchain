#!/bin/bash
#
# call_recovery.sh - Regression test for CALL-target recovery.
#
# Some captured objects contain a CALL whose target offset does not land on an
# instruction boundary (a known compiler defect, e.g. QALCALLF 6.03.17, whose
# second CALL points into the middle of a CJGT). The decompiler must recover
# gracefully: emit `_invalid_target` with an explanatory comment rather than a
# dangling `label_N` that later fails to parse. Such objects cannot round-trip,
# so this is a best-effort (-f) content check, not a round-trip test.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TBOLDC="${TBOLDC:-$SCRIPT_DIR/../tboldc}"
FIXTURE="$SCRIPT_DIR/fixtures/qalcallf-badcall.pgm"

RED='\033[1;31m'; GREEN='\033[1;32m'; NC='\033[0m'
fail=0

out="$(mktemp)"
"$TBOLDC" -f -o "$out" "$FIXTURE" 2>/dev/null

if [ ! -s "$out" ]; then
    echo -e "  ${RED}FAIL${NC}: decompiler produced no output for a mid-instruction CALL"
    fail=1
elif ! grep -q '_invalid_target' "$out"; then
    echo -e "  ${RED}FAIL${NC}: expected _invalid_target recovery marker, not found"
    fail=1
elif grep -qE 'label_[0-9]+ P4, P5;' "$out"; then
    echo -e "  ${RED}FAIL${NC}: emitted a dangling label instead of recovering the CALL"
    fail=1
else
    echo -e "  ${GREEN}PASS${NC}: mid-instruction CALL recovered as _invalid_target"
fi

rm -f "$out"
exit $fail
