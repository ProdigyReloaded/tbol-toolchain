#!/bin/bash
#
# sdb_test.sh - Source-debug-info (.sdb) emission test.
#
# Compiles a fixture with -g and checks the emitted .sdb carries the maps a
# DAP debugger needs (see reception-system docs/SDB-FORMAT.md): the [lines]
# address->source table, the [symbols] name->slot table (with array lengths),
# and the [procs] proc->code-range table. It also asserts proc ranges are
# contiguous and half-open, and that -g does not change the emitted .cod.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TBOLC="${TBOLC:-$SCRIPT_DIR/../tbolc}"
SRC="$SCRIPT_DIR/sdb/two_procs.src"

RED='\033[1;31m'; GREEN='\033[1;32m'; NC='\033[0m'
fail=0
pass() { echo -e "  ${GREEN}PASS${NC}: $1"; }
bad()  { echo -e "  ${RED}FAIL${NC}: $1"; fail=1; }

work="$(mktemp -d)"
"$TBOLC" -g -o "$work" "$SRC" 2>"$work/err" || { bad "compile with -g failed"; cat "$work/err"; exit 1; }
sdb="$(ls "$work"/*.sdb 2>/dev/null | head -1)"
cod_g="$(ls "$work"/*.cod 2>/dev/null | head -1)"
[ -s "$sdb" ] || { bad "no .sdb emitted"; exit 1; }

# --- sections present ---
for sec in '\[files\]' '\[lines\]' '\[procs\]' '\[symbols\]'; do
    grep -qE "^$sec" "$sdb" || bad "missing section ${sec//\\/}"
done
[ "$fail" = 0 ] && pass "all sections present (files, lines, procs, symbols)"

# --- staleness hashes: cod + per-file, 16 lowercase hex, not "0" ---
if grep -qE '^cod[[:space:]]+\S+[[:space:]]+[0-9a-f]{16}$' "$sdb"; then
    pass "cod staleness hash present ($(awk '/^cod /{print $3}' "$sdb"))"
else
    bad "cod hash missing or not 16-hex (got: $(grep '^cod ' "$sdb"))"
fi
if awk '/^0 /{exit !($NF ~ /^[0-9a-f]{16}$/)}' "$sdb"; then
    pass "primary [files] row carries a content hash"
else
    bad "primary [files] row missing a 16-hex content hash: $(grep '^0 ' "$sdb")"
fi

# --- [procs]: MAIN + HELPER, contiguous half-open ranges ---
procs="$(sed -n '/^\[procs\]/,/^\[/p' "$sdb" | grep -iE '^(MAIN|HELPER)')"
main_start=$(echo "$procs" | awk 'toupper($1)=="MAIN"{print $2}')
main_end=$(echo "$procs"   | awk 'toupper($1)=="MAIN"{print $3}')
help_start=$(echo "$procs" | awk 'toupper($1)=="HELPER"{print $2}')
if echo "$procs" | grep -qiE '^MAIN' && echo "$procs" | grep -qiE '^HELPER'; then
    pass "both procs present ($main_start-$main_end, $help_start-...)"
else
    bad "expected MAIN and HELPER in [procs], got: $procs"
fi
if [ -n "$main_end" ] && [ "$main_end" = "$help_start" ]; then
    pass "proc ranges contiguous (main end == helper start: $main_end)"
else
    bad "proc ranges not contiguous (main end $main_end != helper start $help_start)"
fi
[ "$main_start" = "0x0000" ] && pass "main starts at 0x0000" || bad "main should start at 0x0000, got $main_start"

# --- [symbols]: array length captured ---
if grep -qE '^BUF[[:space:]]+RDA[[:space:]]+[0-9]+[[:space:]]+10' "$sdb"; then
    pass "array length captured (BUF ... 10)"
else
    bad "expected BUF with len 10 in [symbols]"
fi

# --- -g must not change the .cod ---
work2="$(mktemp -d)"; "$TBOLC" -o "$work2" "$SRC" 2>/dev/null
cod_plain="$(ls "$work2"/*.cod 2>/dev/null | head -1)"
if cmp -s "$cod_g" "$cod_plain"; then
    pass "-g leaves the .cod unchanged"
else
    bad "-g changed the emitted .cod"
fi

rm -rf "$work" "$work2"
exit $fail
