#!/usr/bin/env bash

set -euo pipefail

top_dir="${MAUDE_SE_TOP_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
build_venv="$top_dir/.build-wheel/venv-build"
test_venv="$top_dir/.build-wheel/venv-test"

# shellcheck source=version.sh
source "$top_dir/build/version.sh"

usage() {
  cat <<'EOF'
Usage: ./build.sh <command>

Commands:
  doctor [standalone]
                Check the local macOS build environment without changing it
  install-deps  Install required Homebrew packages
  setup         Create the build virtualenv and prepare pinned upstream sources
  wheel         Build a macOS wheel into out/
  test          Install the wheel in an isolated environment and run smoke tests
  standalone    Build a self-contained macOS executable ZIP into out/
  test-standalone
                Extract and smoke-test the standalone ZIP
  shell [test|build]
                Open a shell using the test or build virtualenv
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

audit_macos_linkage() {
  local binary="$1"
  local dependency
  local failed=0

  [[ "$(uname -s)" == "Darwin" ]] || return 0
  while IFS= read -r dependency; do
    dependency="${dependency%% (*}"
    case "$dependency" in
    /usr/lib/* | /System/Library/* | @rpath/* | @loader_path/* | @executable_path/*) ;;
    *)
      printf 'external dynamic dependency: %s -> %s\n' "$binary" "$dependency" >&2
      failed=1
      ;;
    esac
  done < <(otool -L "$binary" | tail -n +2 | sed 's/^[[:space:]]*//')

  ((failed == 0)) || fail "non-system dynamic dependencies were found"
}

have_command() {
  command -v "$1" >/dev/null 2>&1
}

doctor() {
  local profile="${1:-wheel}"
  local failed=0
  local command_name
  local formula
  local installed_formulae=""

  if [[ "$(uname -s)" != "Darwin" ]]; then
    fail "local wheel builds currently support macOS only" || true
    failed=1
  fi

  case "$profile" in
  wheel | standalone) ;;
  *) fail "unknown build profile '$profile'; use 'wheel' or 'standalone'" ;;
  esac

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
    local formulae=(bison flex cmake swig)
    if [[ "$profile" == "standalone" ]]; then
      formulae+=(autoconf automake)
      for command_name in autoreconf cmake zip unzip; do
        if have_command "$command_name"; then
          printf 'ok      %s\n' "$command_name"
        else
          printf 'missing %s\n' "$command_name"
          failed=1
        fi
      done
    fi
    for formula in "${formulae[@]}"; do
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

  brew install bison flex autoconf automake cmake swig
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

build_standalone() {
  local version

  doctor standalone
  version="$(maude_se_version)"
  export PATH="$(brew --prefix bison)/bin:$(brew --prefix flex)/bin:$PATH"

  "$top_dir/build/native.sh" prep
  "$top_dir/build/native.sh" deps
  "$top_dir/build/native.sh" build-maude-se "v$version"
  note "standalone artifact is available in $top_dir/out"
}

test_standalone() {
  local archives=("$top_dir"/out/maude_se_z3-*.zip)
  local temp_dir
  local executable
  local bundle_dir
  local output

  if [[ ! -e "${archives[0]}" ]]; then
    fail "no standalone ZIP found in $top_dir/out; run ./build.sh standalone first"
  fi
  if [[ ${#archives[@]} -ne 1 ]]; then
    fail "expected exactly one standalone ZIP in $top_dir/out, found ${#archives[@]}"
  fi

  temp_dir="$(mktemp -d "${TMPDIR:-/tmp}/maude-se-standalone.XXXXXX")"
  trap 'rm -rf -- "$temp_dir"' RETURN
  unzip -q "${archives[0]}" -d "$temp_dir"
  executable="$(find "$temp_dir" -type f -name 'maude-se-z3' -print -quit)"
  [[ -n "$executable" ]] || fail "standalone executable is missing from ${archives[0]}"
  bundle_dir="$(dirname "$executable")"

  output="$(cd "$bundle_dir" && \
    printf 'reduce in NAT : 1 + 1 .\nquit\n' | "$executable")"
  grep -Eq 'result .*: 2' <<<"$output" || fail "standalone calculation smoke test failed"

  cp "$top_dir/examples/smt-check-ex.maude" "$bundle_dir/"
  output="$(cd "$bundle_dir" && \
    printf 'check in SIMPLE : X:Integer > 4 using QF_LRA .\ncheck in SIMPLE : X:Integer > 4 and X:Integer < 3 using QF_LRA .\nquit\n' | \
      "$executable" smt-check-ex.maude smt-check.maude maude-se-meta.maude)"
  grep -Fq 'result: sat' <<<"$output" || fail "standalone Z3 SAT smoke test failed"
  grep -Fq 'result: unsat' <<<"$output" || fail "standalone Z3 UNSAT smoke test failed"

  audit_macos_linkage "$executable"
  note "standalone smoke test passed"
}

test_wheel() {
  local wheels=("$top_dir"/out/*.whl)
  local output
  local solver

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
    "${wheels[0]}[all-solvers]"

  "$test_venv/bin/python" -c 'import maudeSE'
  "$test_venv/bin/maude-se" --help >/dev/null
  for solver in z3 yices cvc5; do
    output="$(
      printf 'check in SIMPLE : X:Integer > 4 using QF_LRA .\ncheck in SIMPLE : X:Integer > 4 and X:Integer < 3 using QF_LRA .\nquit\n' | \
        "$test_venv/bin/maude-se" "$top_dir/examples/smt-check-ex.maude" -s "$solver"
    )"
    grep -Fq 'result: sat' <<<"$output" || fail "wheel $solver SAT smoke test failed"
    grep -Fq 'result: unsat' <<<"$output" || fail "wheel $solver UNSAT smoke test failed"
  done
  while IFS= read -r binary; do
    audit_macos_linkage "$binary"
  done < <(find "$test_venv" -type f \( -name '*.so' -o -name '*.dylib' \) \
    -path '*/maudeSE/*' -print)
  note "wheel smoke tests passed"
}

open_venv_shell() {
  local environment="${1:-test}"
  local venv_dir
  local shell_path="${SHELL:-/bin/bash}"
  local shell_name

  case "$environment" in
  test) venv_dir="$test_venv" ;;
  build) venv_dir="$build_venv" ;;
  *) fail "unknown environment '$environment'; use 'test' or 'build'" ;;
  esac

  if [[ ! -x "$venv_dir/bin/python" ]]; then
    if [[ "$environment" == "test" ]]; then
      fail "test environment not found; run ./build.sh test first"
    else
      fail "build environment not found; run ./build.sh setup first"
    fi
  fi

  export VIRTUAL_ENV="$venv_dir"
  export PATH="$venv_dir/bin:$PATH"
  unset PYTHONHOME 2>/dev/null || true
  shell_name="$(basename "$shell_path")"

  note "opening $environment environment; run 'exit' to return"
  case "$shell_name" in
  zsh) exec "$shell_path" -f -i ;;
  bash) exec "$shell_path" --noprofile --norc -i ;;
  *) exec "$shell_path" -i ;;
  esac
}

clean_build() {
  local path
  local paths=(
    "$top_dir/.build-wheel"
    "$top_dir/.build-standalone"
    "$top_dir/out"
  )

  for path in "${paths[@]}"; do
    [[ "$path" == "$top_dir"/* ]] || fail "refusing to remove unsafe path: $path"
    rm -rf -- "$path"
  done
  note "local build artifacts removed"
}
