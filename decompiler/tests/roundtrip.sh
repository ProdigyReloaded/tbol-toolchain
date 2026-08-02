#!/bin/bash
# roundtrip.sh - Test decompiler round-trip on verified corpus
#
# Only tests programs whose source compiles to bytecode identical to the
# original (i.e., programs that pass "make check"). For each: decompile
# the original bytecode -> recompile -> compare bytecodes.

TBOLDC=$(dirname "$0")/../tboldc
TBOLC=$(dirname "$0")/../../compiler/tbolc
WORKDIR=$(mktemp -d)

APPS_DIR="${1:-$(dirname "$0")/../../../applications}"
INCLUDE_DIR="$APPS_DIR/common"

pass=0
fail=0
error=0
skip=0
total=0

# Collect verified programs: run make check in each app directory,
# record .COD paths that report IDENTICAL.
verified=""
for dir in $(find "$APPS_DIR" -name "Makefile" -path "*/v[12]/*" -exec dirname {} \;); do
    [ -d "$dir/src" ] || continue
    while IFS= read -r line; do
        # Lines look like: "build/FOO.COD vs orig/FOO.PGM... IDENTICAL"
        cod_rel=$(echo "$line" | sed 's/ vs .*//')
        verified="$verified $dir/$cod_rel"
    done < <(cd "$dir" && make check 2>/dev/null | grep "IDENTICAL")
done

for cod in $verified; do
    [ -f "$cod" ] || continue
    total=$((total + 1))
    name=$(basename "$cod" .COD)

    # Decompile
    if ! "$TBOLDC" -I "$INCLUDE_DIR" "$cod" -o "$WORKDIR/test.src" 2>/dev/null; then
        echo "DECOMPILE_ERROR: $name"
        error=$((error + 1))
        continue
    fi

    # Recompile
    if ! "$TBOLC" -I "$INCLUDE_DIR" "$WORKDIR/test.src" -o "$WORKDIR/" 2>/dev/null; then
        echo "COMPILE_ERROR: $name"
        error=$((error + 1))
        rm -f "$WORKDIR/test.src" "$WORKDIR/test.cod"
        continue
    fi

    # Compare bytecodes (skip header - code_offset is at bytes 2-3, big-endian)
    code_off=$(od -An -j2 -N2 -tu2 --endian=big "$cod" 2>/dev/null | tr -d ' ')
    [ -z "$code_off" ] && code_off=34
    diffs=$(cmp -l "$cod" "$WORKDIR/test.cod" 2>/dev/null | awk -v off="$code_off" '$1 > off {print}' | wc -l)
    if [ "$diffs" -eq 0 ]; then
        pass=$((pass + 1))
    else
        echo "MISMATCH: $name ($diffs byte diffs)"
        fail=$((fail + 1))
    fi

    rm -f "$WORKDIR/test.src" "$WORKDIR/test.cod"
done

rm -rf "$WORKDIR"

echo ""
echo "=== Results ==="
echo "Total: $total"
echo "Pass:  $pass"
echo "Fail:  $fail"
echo "Error: $error"

[ "$fail" -eq 0 ] && [ "$error" -eq 0 ] && exit 0 || exit 1
