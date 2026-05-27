#!/bin/bash
#
# Generate reference .cod files for all positive test cases.
#
# Compiles each .src with tbolc and places the output in reference/.
#
# Usage:
#   ./gen_reference.sh                      # Generate tbolc reference files
#   ./gen_reference.sh positive/verbs       # One category only

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TBOLC="${TBOLC:-$SCRIPT_DIR/../tbolc}"

target="${1:-}"

gen_tbolc_reference() {
    local src="$1"
    local dir="$(dirname "$src")"
    local base="$(basename "$src" .src)"
    base="$(echo "$base" | tr '[:upper:]' '[:lower:]')"

    local include_args=()
    if [ -d "$dir/includes" ]; then
        include_args+=(-I "$dir/includes")
    fi

    local workdir
    workdir="$(mktemp -d)"

    if "$TBOLC" ${include_args[@]+"${include_args[@]}"} -o "$workdir" "$src" >/dev/null 2>&1; then
        local cod
        cod="$(ls "$workdir"/*.cod 2>/dev/null | head -1)"
        if [ -n "$cod" ]; then
            mkdir -p "$dir/reference"
            cp "$cod" "$dir/reference/${base}.cod"
            echo "  tbolc: $src -> reference/${base}.cod"
        fi
    else
        echo "  tbolc: $src FAILED (skipping)"
    fi

    rm -rf "$workdir"
}

search_dir="$SCRIPT_DIR/positive"
if [ -n "$target" ]; then
    search_dir="$SCRIPT_DIR/$target"
fi

echo "Generating reference files..."
echo ""

while IFS= read -r src; do
    gen_tbolc_reference "$src"
done < <(find "$search_dir" -name '*.src' -o -name '*.SRC' | sort)

echo ""
echo "Done."
