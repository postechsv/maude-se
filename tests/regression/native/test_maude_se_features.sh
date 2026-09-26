#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 || ! -x "$1" ]]; then
  echo "usage: $0 /path/to/maude-se-{z3,yices,cvc5}" >&2
  exit 2
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_dir="$(cd "$script_dir/../../.." && pwd)"
binary="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
bundle_dir="$(dirname "$binary")"

check_output() {
  local label="$1"
  local output="$2"
  shift 2
  if grep -Eq 'Warning:|Error:|Parse error|Maude internal error' <<< "$output"; then
    echo "$label failed:" >&2
    echo "$output" >&2
    exit 1
  fi
  local expected
  for expected in "$@"; do
    if ! grep -Fq "$expected" <<< "$output"; then
      echo "$label: expected '$expected' in output:" >&2
      echo "$output" >&2
      exit 1
    fi
  done
}

output="$(printf '%s\n' \
  'check in SIMPLE : X:Integer > 4 using QF_LRA .' \
  'check in SIMPLE : X:Integer > 4 and X:Integer < 3 using QF_LRA .' \
  'quit' | MAUDE_LIB="$bundle_dir:$repo_dir/src" "$binary" -no-banner \
  "$repo_dir/examples/smt-check-ex.maude" \
  "$bundle_dir/smt-check.maude" "$bundle_dir/maude-se-meta.maude" 2>&1)"
check_output 'SMT satisfiability' "$output" 'result: sat' 'result: unsat'

output="$(printf '%s\n' \
  'check in SIMPLE : X:Integer === 7 using QF_LRA .' \
  'show model .' \
  'quit' | MAUDE_LIB="$bundle_dir:$repo_dir/src" "$binary" -no-banner \
  "$repo_dir/examples/smt-check-ex.maude" \
  "$bundle_dir/smt-check.maude" "$bundle_dir/maude-se-meta.maude" 2>&1)"
check_output 'SMT model' "$output" 'result: sat' 'X:Integer |--> (7).Integer'

output="$(printf '%s\n' \
  'smt-search [1] in GCD : gcd(10, I:Integer) =>* return(2) such that I:Integer === 6 using QF_LRA .' \
  'quit' | MAUDE_LIB="$bundle_dir:$repo_dir/src" "$binary" -no-banner \
  "$repo_dir/examples/smt-search-ex.maude" \
  "$bundle_dir/smt-check.maude" "$bundle_dir/maude-se-meta.maude" 2>&1)"
check_output 'symbolic GCD reachability' "$output" 'Solution 1' 'Concrete state:' 'return(2)' 'I:Integer <-- 6'

output="$(printf '%s\n' \
  'smt-search [1] in GCD : gcd(10, I:Integer) =>* return(3) such that I:Integer === 6 using QF_LRA .' \
  'quit' | MAUDE_LIB="$bundle_dir:$repo_dir/src" "$binary" -no-banner \
  "$repo_dir/examples/smt-search-ex.maude" \
  "$bundle_dir/smt-check.maude" "$bundle_dir/maude-se-meta.maude" 2>&1)"
if grep -Fq 'Solution 1' <<< "$output" ||
   ! grep -Eq 'No (more )?solutions?\.' <<< "$output"; then
  echo "unsatisfiable symbolic GCD reachability failed:" >&2
  echo "$output" >&2
  exit 1
fi
check_output 'unsatisfiable symbolic GCD reachability' "$output"

output="$(printf 'quit\n' | MAUDE_LIB="$bundle_dir:$repo_dir/src" \
  "$binary" -no-banner "$repo_dir/tests/data/maude-se-2020/gcd.maude" 2>&1)"
check_output '2020 Core Maude GCD' "$output" 'Solution 1' 'NN:IntegerExpr -->'

# Exercise repeated conversion of shared SMT subexpressions within one process.
output="$(printf '%s\n' \
  "smt-search [1,40] [ l0 : 0/1 ] =>* [ bad : X' ] ." \
  "smt-search [1,160] [ l0 : 0/1 ] =>* [ bad : X' ] ." \
  'quit' | MAUDE_LIB="$bundle_dir:$repo_dir/tests/data/pta2maude:$repo_dir/src" \
  "$binary" -no-banner "$repo_dir/tests/data/pta2maude/ex-fig3b.maude" 2>&1)"
check_output 'repeated PTA SMT search' "$output" 'rewrites: 40' 'rewrites: 160'
if [[ "$(grep -Fc 'No solution.' <<< "$output")" -ne 3 ]]; then
  echo "repeated PTA SMT search returned an unexpected result:" >&2
  echo "$output" >&2
  exit 1
fi
