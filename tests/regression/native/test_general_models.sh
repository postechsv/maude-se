#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 /path/to/maude-se-{z3,yices,cvc5}" >&2
  exit 2
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd "$script_dir/../../.." && pwd)"
binary="$1"
bundle_dir="$(dirname "$binary")"

output="$(MAUDE_LIB="$bundle_dir:$repo_dir/src" "$binary" -no-banner 2>&1 <<EOF
load $repo_dir/tests/data/general/bakery/bakery.maude .
mod BAKERY-TEST is protecting BAKERY . eq @K = 3 . endm
search [1] in BAKERY-TEST : 0 ; 0 ; [idle] [idle] =>* N:Nat ; M:Nat ; [crit(0)] PS:ProcSet .
load $repo_dir/tests/data/general/dining-philosophers/dining-philosophers.maude .
mod DINING-TEST is protecting N-DINING-PHILOSOPHERS . eq #N = 3 . endm
search [1] in DINING-TEST : p(0, think) || c(0) || c(1) =>* p(0, eat) .
quit
EOF
)"

if grep -Eq 'Warning:|Error:' <<< "$output"; then
  echo "$output" >&2
  exit 1
fi
if [[ "$(grep -c 'Solution 1' <<< "$output")" -ne 2 ]]; then
  echo "Expected reachable critical section and eating state:" >&2
  echo "$output" >&2
  exit 1
fi
