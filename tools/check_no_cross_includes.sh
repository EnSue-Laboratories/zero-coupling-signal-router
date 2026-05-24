#!/usr/bin/env bash
# Zero-coupling guard: a module may include ONLY shared "zcsr/..." contracts,
# never another module's path or a relative "../" include.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
fail=0

while IFS= read -r f; do
  if grep -nE '#include[[:space:]]*[<"](\.\./|modules/)' "$f"; then
    echo "  ^ CROSS-MODULE / RELATIVE INCLUDE in: $f"
    fail=1
  fi
done < <(find "$root/modules" -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \))

if [ "$fail" -ne 0 ]; then
  echo "FAIL: cross-module includes found (modules must include only zcsr/* contracts)"
  exit 1
fi
echo "OK: modules include only shared contracts (zero-coupling intact)"
