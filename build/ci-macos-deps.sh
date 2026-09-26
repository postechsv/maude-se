#!/usr/bin/env bash
set -euo pipefail

top_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
archive="$top_dir/.ci-deps/macos-deps.tar.gz"
cd "$top_dir"

case "${1:-}" in
prepare)
  ./build.sh install-deps
  export PATH="$(brew --prefix bison)/bin:$(brew --prefix flex)/bin:$(brew --prefix autoconf)/bin:$(brew --prefix automake)/bin:$PATH"

  MAUDE_SE_WHEEL_DEPS_DIR="$top_dir/.build-wheel" ./build/build.sh deps
  for solver in z3 cvc5 yices; do
    ./build/native.sh deps "$solver"
  done

  # Only installed libraries, signatures, and packaged solver licenses are
  # needed by the downstream jobs; source/build trees would make this large.
  # shellcheck source=versions.env
  source build/versions.env
  arch="$(uname -m)"
  mkdir -p "$(dirname "$archive")"
  tar -czf "$archive" \
    .build-wheel/.build-signature .build-wheel/install \
    .build-standalone/.common-deps-signature \
    .build-standalone/.z3-deps-signature \
    .build-standalone/.cvc5-deps-signature \
    .build-standalone/.yices-deps-signature \
    .build-standalone/install \
    ".build-standalone/dependencies/cvc5-macOS-$arch-static/COPYING" \
    ".build-standalone/dependencies/cvc5-macOS-$arch-static/licenses" \
    ".build-standalone/dependencies/yices-Darwin-$arch/yices-$YICES_VERSION/LICENSE" \
    ".build-standalone/dependencies/yices-Darwin-$arch/yices-$YICES_VERSION/NOTICES" \
    .build-standalone/dependencies/cudd-3.0.0/LICENSE
  ;;
restore)
  [[ -f "$archive" ]] || {
    echo "error: macOS dependency artifact is missing: $archive" >&2
    exit 1
  }
  tar -xzf "$archive"
  ;;
*)
  echo "usage: $0 prepare|restore" >&2
  exit 2
  ;;
esac
