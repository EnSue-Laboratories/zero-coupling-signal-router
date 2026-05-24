#!/usr/bin/env bash
# Zero-coupling guard: a module may include ONLY shared "zcsr/..." contracts,
# never another module's path or a relative "../" include.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
fail=0

while IFS= read -r f; do
  # Flag cross-module / relative includes, but allow the one sanctioned vendored dependency
  # (third_party single-header, per third_party/README.md).
  if grep -nE '#include[[:space:]]*[<"](\.\./|modules/)' "$f" | grep -vE 'third_party/'; then
    echo "  ^ CROSS-MODULE / RELATIVE INCLUDE in: $f"
    fail=1
  fi
done < <(find "$root/modules" -type f \( -name '*.c' -o -name '*.m' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \))

if [ "$fail" -ne 0 ]; then
  echo "FAIL: cross-module includes found (modules must include only zcsr/* contracts)"
  exit 1
fi
echo "OK: modules include only shared contracts (zero-coupling intact)"
