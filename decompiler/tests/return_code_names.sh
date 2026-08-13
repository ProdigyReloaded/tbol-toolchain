#!/bin/bash
#
# return_code_names.sh - Regression test for SYS_RETURN_CODE (RET_*) resolution.
#
# SYS_RETURN_CODE (GEV #1) compared against a canonical decimal literal renders
# with the matching RET_* name ('0' -> RET_OK, '10' -> RET_SOME_ONES). The
# literal must round-trip: an EMPTY string '' must NOT resolve to RET_OK, even
# though atoi("") == atoi("0") == 0. Emitting RET_OK for '' is both semantically
# wrong and breaks round-tripping ('' is 0 bytes, RET_OK='0' is 1), shifting
# every later offset (observed in QAPLTEXT 6.03.17).
#
# The fixture (compiler control positive test) compares SYS_RETURN_CODE against
# '', '0', and '10'. Decompiled WITHOUT -f, so a clean round-trip is required.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TBOLDC="${TBOLDC:-$SCRIPT_DIR/../tboldc}"
REF="$SCRIPT_DIR/../../compiler/tests/positive/control/reference"
FIXTURE="$REF/ret_codes.cod"

RED='\033[1;31m'; GREEN='\033[1;32m'; NC='\033[0m'
fail=0

out="$(mktemp)"
"$TBOLDC" -o "$out" "$FIXTURE" 2>/dev/null

if [ ! -s "$out" ]; then
    echo -e "  ${RED}FAIL${NC}: no output (round-trip verification failed?)"
    fail=1
elif ! grep -q "SYS_RETURN_CODE = ''" "$out"; then
    echo -e "  ${RED}FAIL${NC}: empty-string operand not preserved (wrongly resolved to a RET_* name?)"
    fail=1
elif ! grep -q "= RET_OK" "$out"; then
    echo -e "  ${RED}FAIL${NC}: '0' did not resolve to RET_OK"
    fail=1
elif ! grep -q "= RET_SOME_ONES" "$out"; then
    echo -e "  ${RED}FAIL${NC}: '10' did not resolve to RET_SOME_ONES"
    fail=1
elif [ "$(grep -c "RET_OK" "$out")" -ne 1 ]; then
    echo -e "  ${RED}FAIL${NC}: RET_OK appears $(grep -c "RET_OK" "$out") times (expected 1; '' must not become RET_OK)"
    fail=1
else
    echo -e "  ${GREEN}PASS${NC}: '' preserved; '0'->RET_OK, '10'->RET_SOME_ONES; round-trip clean"
fi

rm -f "$out"
exit $fail
