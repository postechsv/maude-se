#!/usr/bin/env bash

maude_se_version() {
  local pyproject="$top_dir/src/pyproject.toml"
  local version

  version="$(sed -n "s/^version[[:space:]]*=[[:space:]]*'\([^']*\)'.*/\1/p" "$pyproject")"
  [[ -n "$version" ]] || {
    echo "error: could not read the MaudeSE version from $pyproject" >&2
    return 1
  }
  printf '%s\n' "$version"
}

maude_se_release_tag() {
  printf 'v%s\n' "$(maude_se_version)"
}

validate_maude_se_release_tag() {
  local actual="$1"
  local expected

  expected="$(maude_se_release_tag)"
  [[ "$actual" == "$expected" ]] || {
    echo "error: release tag $actual does not match src/pyproject.toml version $expected" >&2
    return 1
  }
}

prepare_maude_se_package_sources() {
  local destination="$1"
  local version
  local built_date

  version="$(maude_se_version)"
  built_date="$(LC_ALL=C date '+%B %e %Y' | sed 's/  / /g')"

  rm -rf "$destination"
  mkdir -p "$destination"
  cp -R "$top_dir/src/pysmt" "$destination/pysmt"
  cp "$top_dir/src/smt-check.maude" "$destination/smt-check.maude"
  sed \
    -e "s/@MAUDE_SE_VERSION@/$version/g" \
    -e "s/@MAUDE_SE_BUILT@/$built_date/g" \
    "$top_dir/src/maude-se-meta.maude" >"$destination/maude-se-meta.maude"
}
