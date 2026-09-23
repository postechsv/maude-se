#!/usr/bin/env bash

set -euo pipefail

top_dir="${MAUDE_SE_TOP_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
build_venv="$top_dir/.venv-build"
test_venv="$top_dir/.venv-test"

usage() {
  cat <<'EOF'
Usage: ./build.sh <command>

Commands:
  doctor        Check the local macOS build environment without changing it
  install-deps  Install required Homebrew packages
  setup         Create the build virtualenv and prepare pinned upstream sources
  wheel         Build a macOS wheel into out/
  test          Install the wheel in an isolated environment and run smoke tests
  clean         Remove generated local build directories
  help          Show this help
EOF
}

note() {
  printf '==> %s\n' "$*"
}

fail() {
  printf 'error: %s\n' "$*" >&2
  return 1
}

have_command() {
  command -v "$1" >/dev/null 2>&1
}

doctor() {
  local failed=0
  local command_name
  local formula
  local installed_formulae=""

  if [[ "$(uname -s)" != "Darwin" ]]; then
    fail "local wheel builds currently support macOS only" || true
    failed=1
  fi

  for command_name in git curl tar make clang clang++ python3 brew; do
    if have_command "$command_name"; then
      printf 'ok      %s\n' "$command_name"
    else
      printf 'missing %s\n' "$command_name"
      failed=1
    fi
  done

  if have_command xcrun && xcrun --show-sdk-path >/dev/null 2>&1; then
    printf 'ok      macOS SDK\n'
  else
    printf 'missing macOS SDK (run: xcode-select --install)\n'
    failed=1
  fi

  if have_command python3; then
    if python3 -c 'import sys; raise SystemExit(sys.version_info < (3, 8))'; then
      printf 'ok      Python >= 3.8\n'
    else
      printf 'missing Python >= 3.8\n'
      failed=1
    fi
  fi

  if have_command brew; then
    installed_formulae="$(brew list --formula -1 2>/dev/null || true)"
    for formula in bison flex gmp libsigsegv libtecla; do
      if grep -Fxq "$formula" <<<"$installed_formulae"; then
        printf 'ok      brew:%s\n' "$formula"
      else
        printf 'missing brew:%s\n' "$formula"
        failed=1
      fi
    done
  fi

  if ((failed)); then
    printf '\nInstall the missing Homebrew packages with:\n'
    printf '  ./build.sh install-deps\n'
    return 1
  fi

  note "local build environment is ready"
}

install_deps() {
  [[ "$(uname -s)" == "Darwin" ]] || fail "install-deps supports macOS only"
  have_command brew || fail "Homebrew is required: https://brew.sh"

  brew install bison flex gmp libsigsegv libtecla
  note "Homebrew dependencies are installed"
}

ensure_build_venv() {
  if [[ ! -x "$build_venv/bin/python" ]]; then
    note "creating isolated Python build environment"
    python3 -m venv "$build_venv"
  fi

  "$build_venv/bin/python" -m pip install --disable-pip-version-check \
    "pip==25.0.1"
  "$build_venv/bin/python" -m pip install --disable-pip-version-check \
    -r "$top_dir/build/requirements.txt"

  export PATH="$build_venv/bin:$(brew --prefix bison)/bin:$(brew --prefix flex)/bin:$PATH"
}

setup_build() {
  doctor
  ensure_build_venv
  "$top_dir/build/build.sh" prep
}

build_wheel() {
  setup_build
  "$top_dir/build/build.sh" deps
  "$top_dir/build/build.sh" build-maude
  "$top_dir/build/build.sh" build-maude-se
  note "wheel artifacts are available in $top_dir/out"
}

test_wheel() {
  local wheels=("$top_dir"/out/*.whl)

  if [[ ! -e "${wheels[0]}" ]]; then
    fail "no wheel found in $top_dir/out; run ./build.sh wheel first"
  fi
  if [[ ${#wheels[@]} -ne 1 ]]; then
    fail "expected exactly one wheel in $top_dir/out, found ${#wheels[@]}"
  fi

  note "creating isolated smoke-test environment"
  python3 -m venv --clear "$test_venv"
  "$test_venv/bin/python" -m pip install --disable-pip-version-check \
    "pip==25.0.1"
  "$test_venv/bin/python" -m pip install --disable-pip-version-check \
    "${wheels[0]}" "pyyaml==6.0.3" "z3-solver==4.13.0.0"

  "$test_venv/bin/python" -c 'import maudeSE'
  "$test_venv/bin/maude-se" --help >/dev/null
  printf 'quit\n' | \
    "$test_venv/bin/maude-se" "$top_dir/examples/smt-check-ex.maude" -s z3
  note "wheel smoke tests passed"
}

clean_build() {
  local path
  local paths=(
    "$top_dir/.build"
    "$top_dir/.3rd_party"
    "$top_dir/.venv-build"
    "$top_dir/.venv-test"
    "$top_dir/maude-bindings"
    "$top_dir/out"
  )

  for path in "${paths[@]}"; do
    [[ "$path" == "$top_dir"/* ]] || fail "refusing to remove unsafe path: $path"
    rm -rf -- "$path"
  done
  note "local build artifacts removed"
}
