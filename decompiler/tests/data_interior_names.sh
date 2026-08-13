#!/bin/bash
#
# data_interior_names.sh - Regression test for DATA array-vs-scalar overlap.
#
# When a slot is inferred as an array (indexed access) but an interior slot
# of that span is ALSO referenced directly by name, emitting the base as a
# dimensioned array RDA_base(dim) absorbs the interior slot and leaves its
# name undefined - the best-effort source then fails to recompile and the
# round-trip aborts (empty output without -f). ENPHSTAC (STAGE.DAT 6.03.17)
# hits this: RDA36 is indexed while RDA40 is used as a scalar, so RDA36(5)
# swallowed RDA40 ("undefined variable 'RDA40'").
#
# The fix declares such spans slot-by-slot (bytecode-neutral: TBOL indexing
# is slot arithmetic off a scalar declaration). This fixture must therefore
# round-trip WITHOUT -f (non-empty output == recompiled bytecode matched),
# and its DATA section must declare the interior slot by name.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TBOLDC="${TBOLDC:-$SCRIPT_DIR/../tboldc}"
FIXTURE="$SCRIPT_DIR/fixtures/enphstac-array-interior.pgm"

RED='\033[1;31m'; GREEN='\033[1;32m'; NC='\033[0m'
fail=0

out="$(mktemp)"
# No -f: output is written only if the round-trip verified.
"$TBOLDC" -o "$out" "$FIXTURE" 2>/dev/null

if [ ! -s "$out" ]; then
    echo -e "  ${RED}FAIL${NC}: ENPHSTAC did not round-trip (empty output)"
    fail=1
elif ! sed -n '/^DATA/,/^PROC/p' "$out" | grep -qE '^[[:space:]]*RDA40,?[[:space:]]*$'; then
    echo -e "  ${RED}FAIL${NC}: interior slot RDA40 not declared individually in DATA"
    fail=1
else
    echo -e "  ${GREEN}PASS${NC}: array/scalar-overlap DATA round-trips with interior names"
fi

rm -f "$out"
exit $fail
