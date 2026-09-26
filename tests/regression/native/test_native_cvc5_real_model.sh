#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 /path/to/maude-se-cvc5" >&2
  exit 2
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd "$script_dir/../../.." && pwd)"
binary="$1"
bundle_dir="$(dirname "$binary")"

output="$(printf 'quit\n' | MAUDE_LIB="$bundle_dir:$repo_dir/src" "$binary" "$repo_dir/tests/data/smoke/cvc5-real-model.maude")"
if ! grep -Fq "'X:Real |-> '0/1.Real" <<< "$output"; then
  echo "cvc5 did not preserve the Real sort in its model:" >&2
  echo "$output" >&2
  exit 1
fi
