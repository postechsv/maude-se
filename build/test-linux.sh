#!/usr/bin/env bash

set -euo pipefail

top_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mode="${1:-}"

case "$mode" in
wheel)
  versions=(cp310-cp310 cp311-cp311 cp312-cp312 cp313-cp313 cp314-cp314)
  for version in "${versions[@]}"; do
    if [[ ! -x "/opt/python/$version/bin/python" ]]; then
      echo "error: required Python is unavailable: $version" >&2
      exit 1
    fi
  done
  for version in "${versions[@]}"; do
    wheels=("$top_dir"/out/maude_se-*-"$version"-manylinux_*.whl)
    [[ ${#wheels[@]} -eq 1 && -f "${wheels[0]}" ]] || {
      echo "error: expected one $version manylinux wheel" >&2
      exit 1
    }

    venv="$top_dir/.build-wheel/venv-test-$version"
    "/opt/python/$version/bin/python" -m venv --clear "$venv"
    "$venv/bin/python" -m pip install --disable-pip-version-check "${wheels[0]}"
    if output="$("$venv/bin/maude-se-installer" doctor z3)"; then
      echo "error: base $version wheel unexpectedly includes Z3" >&2
      exit 1
    fi
    grep -Fq 'z3-solver is not installed' <<<"$output" || {
      echo "error: base $version wheel did not report missing Z3" >&2
      exit 1
    }
    if output="$("$venv/bin/maude-se" "$top_dir/examples/smt-check-ex.maude" 2>&1)"; then
      echo "error: base $version wheel ran without Z3" >&2
      exit 1
    fi
    grep -Fq 'maude-se-installer install z3' <<<"$output" || {
      echo "error: base $version wheel did not explain how to install Z3" >&2
      exit 1
    }
    "$venv/bin/python" -m pip install --disable-pip-version-check "${wheels[0]}[all-solvers]"
    "$venv/bin/python" -m pip check
    "$venv/bin/maude-se-installer" doctor

    for solver in z3 yices cvc5; do
      output="$(
        printf 'check in SIMPLE : X:Integer > 4 using QF_LRA .\ncheck in SIMPLE : X:Integer > 4 and X:Integer < 3 using QF_LRA .\nquit\n' |
          "$venv/bin/maude-se" "$top_dir/examples/smt-check-ex.maude" -s "$solver"
      )"
      grep -Fq 'result: sat' <<<"$output" || {
        echo "error: $version $solver SAT test failed" >&2
        exit 1
      }
      grep -Fq 'result: unsat' <<<"$output" || {
        echo "error: $version $solver UNSAT test failed" >&2
        exit 1
      }
    done
  done
  ;;
standalone)
  "$top_dir/build.sh" test-standalone
  ;;
*)
  echo "usage: $0 wheel|standalone" >&2
  exit 2
  ;;
esac
