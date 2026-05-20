#!/usr/bin/env bash
# Copy MSYS2 DLL dependencies of executables to a destination directory.
# Usage: $0 <dest-dir> <executable> [executable...]
# Uses ldd to find transitive dependencies, filtering for MSYS2 UCRT64 paths.

set -euo pipefail

if [ $# -lt 2 ]; then
    echo "Usage: $0 <dest-dir> <executable> [executable...]"
    exit 1
fi

DEST="$1"
shift

declare -A SEEN
PENDING=("$@")

while [ ${#PENDING[@]} -gt 0 ]; do
    BINARY="${PENDING[0]}"
    PENDING=("${PENDING[@]:1}")

    if [ ! -f "$BINARY" ]; then
        continue
    fi

    while IFS= read -r line; do
        if [[ "$line" =~ \=\>\ (/ucrt64/bin/[^\ ]+\.dll) ]]; then
            DLL_PATH="${BASH_REMATCH[1]}"
            DLL_NAME="$(basename "$DLL_PATH")"

            if [ -z "${SEEN[$DLL_NAME]:-}" ]; then
                SEEN[$DLL_NAME]=1
                cp "$DLL_PATH" "$DEST/"
                PENDING+=("$DEST/$DLL_NAME")
            fi
        elif [[ "$line" =~ \=\>\ (/mingw64/bin/[^\ ]+\.dll) ]]; then
            DLL_PATH="${BASH_REMATCH[1]}"
            DLL_NAME="$(basename "$DLL_PATH")"

            if [ -z "${SEEN[$DLL_NAME]:-}" ]; then
                SEEN[$DLL_NAME]=1
                cp "$DLL_PATH" "$DEST/"
                PENDING+=("$DEST/$DLL_NAME")
            fi
        fi
    done < <(ldd "$BINARY" 2>/dev/null || true)
done

echo "Copied ${#SEEN[@]} DLL dependencies to $DEST"
