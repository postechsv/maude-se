#!/usr/bin/env bash

source_archive_sha256() {
  case "$1" in
  gmp-6.3.0) printf '%s\n' "$GMP_6_3_0_SHA256" ;;
  gmp-6.1.2) printf '%s\n' "$GMP_6_1_2_SHA256" ;;
  libsigsegv-2.15) printf '%s\n' "$LIBSIGSEGV_2_15_SHA256" ;;
  libsigsegv-2.12) printf '%s\n' "$LIBSIGSEGV_2_12_SHA256" ;;
  ncurses-6.5) printf '%s\n' "$NCURSES_6_5_SHA256" ;;
  buddy-2.4) printf '%s\n' "$BUDDY_2_4_SHA256" ;;
  libtecla-1.6.3) printf '%s\n' "$TECLA_1_6_3_SHA256" ;;
  *) printf 'error: no SHA-256 pinned for %s\n' "$1" >&2; return 1 ;;
  esac
}

verify_source_archive() {
  local archive="$1"
  local expected="$2"
  local actual

  if command -v sha256sum >/dev/null 2>&1; then
    actual="$(sha256sum "$archive")"
  elif command -v shasum >/dev/null 2>&1; then
    actual="$(shasum -a 256 "$archive")"
  else
    echo 'error: SHA-256 checker is unavailable' >&2
    return 1
  fi
  actual="${actual%% *}"
  if [[ "$actual" != "$expected" ]]; then
    printf 'error: SHA-256 mismatch for %s (expected %s, got %s)\n' \
      "$archive" "$expected" "$actual" >&2
    return 1
  fi
}

download_source_archive() {
  local url="$1"
  local archive="$2"
  local source_name="$3"
  local expected

  expected="$(source_archive_sha256 "$source_name")" || return 1
  curl -fLsS --retry 3 -o "$archive" "$url"
  verify_source_archive "$archive" "$expected"
}
