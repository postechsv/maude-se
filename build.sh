#!/usr/bin/env bash

set -euo pipefail

top_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export MAUDE_SE_TOP_DIR="$top_dir"

# shellcheck source=build/local.sh
source "$top_dir/build/local.sh"

command="${1:-help}"
if [[ $# -gt 0 ]]; then
  shift
fi

case "$command" in
doctor) doctor "$@" ;;
install-deps) install_deps "$@" ;;
setup | prep) setup_build "$@" ;;
wheel) build_wheel "$@" ;;
test) test_wheel "$@" ;;
standalone) build_standalone "$@" ;;
test-standalone) test_standalone "$@" ;;
shell) open_venv_shell "$@" ;;
clean) clean_build "$@" ;;
deps | patch | build-maude | build-maude-se | prep-build-maude-se | make-patch)
  "$top_dir/build/build.sh" "$command" "$@"
  ;;
help | -h | --help) usage ;;
*)
  echo "Unknown command: $command" >&2
  usage >&2
  exit 2
  ;;
esac
