#!/usr/bin/env bash
set -euo pipefail

top_dir="${MAUDE_SE_TOP_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
source "$top_dir/build/versions.env"
source "$top_dir/build/version.sh"

solver="${1:-all}"
if [[ "$solver" == all ]]; then
  for item in z3 yices cvc5; do bash "$0" "$item"; done
  exit
fi
case "$solver" in z3 | yices | cvc5) ;; *) echo "usage: $0 z3|yices|cvc5|all" >&2; exit 2 ;; esac

source_dir="$top_dir/.build-wheel/sources/maude-bindings/subprojects/maudesmc"
release_dir="$source_dir/release"
native_prefix="${MAUDE_SE_NATIVE_PREFIX:-$top_dir/.build-standalone/install}"
build_python="$top_dir/.build-wheel/venv-build/bin/python"
[[ -x "$build_python" ]] || { echo "error: build virtualenv is missing" >&2; exit 1; }
python_include="$($build_python -c 'import sysconfig; print(sysconfig.get_path("include"))')"
project_version="$(maude_se_version)"
plugin_version="$(sed -n 's/^version = "\([^"]*\)"/\1/p' "$top_dir/src/native_plugins/$solver/pyproject.toml")"
[[ "$plugin_version" == "$project_version" ]] || {
  echo "error: native $solver plugin version $plugin_version differs from maude-se $project_version" >&2
  exit 1
}

if [[ "$(uname -s)" == Darwin ]]; then
  suffix=dylib
  core_library="$release_dir/libmaude.dylib"
  link_mode=(-dynamiclib "-Wl,-install_name,@rpath/libmaude_se_$solver.dylib" \
    -Wl,-rpath,@loader_path/../maudeSE/maude)
  if [[ "$solver" == yices ]]; then
    target="${MAUDE_SE_MACOS_DEPLOYMENT_TARGET:-13.0}"
  elif [[ "$(uname -m)" == arm64 ]]; then
    target="${MAUDE_SE_MACOS_DEPLOYMENT_TARGET:-11.0}"
  else
    target="${MAUDE_SE_MACOS_DEPLOYMENT_TARGET:-10.13}"
  fi
  link_mode+=("-mmacosx-version-min=$target")
else
  suffix=so
  core_library="$release_dir/libmaude.so"
  link_mode=(-shared '-Wl,-rpath,$ORIGIN/../maudeSE/maude')
  target=""
fi
[[ -f "$core_library" ]] || { echo "error: build the base wheel first: $core_library" >&2; exit 1; }

case "$solver" in
  z3)
    source_file=z3.cc
    plugin_define=MAUDE_SE_PLUGIN_Z3
    archives=(libz3.a)
    ;;
  yices)
    source_file=yices2.cc
    plugin_define=MAUDE_SE_PLUGIN_YICES
    archives=(libyices.a libpicpoly.a libcudd.a libgmp.a)
    ;;
  cvc5)
    source_file=cvc5.cc
    plugin_define=MAUDE_SE_PLUGIN_CVC5
    archives=(libcvc5.a libpicpolyxx.a libpicpoly.a libcadical.a libmpfr.a libgmpxx.a libgmp.a)
    ;;
esac
archive_paths=()
for archive in "${archives[@]}"; do
  path="$native_prefix/lib/$archive"
  [[ -f "$path" ]] || { echo "error: missing $path" >&2; exit 1; }
  archive_paths+=("$path")
done

stage="$(mktemp -d "$top_dir/.build-wheel/native-plugin-${solver}.XXXXXX")"
cp "$top_dir/src/native_plugins/$solver/pyproject.toml" "$stage/"
cp "$top_dir/src/native_plugins/setup.py" "$stage/"
cp "$top_dir/LICENSE" "$stage/"
cp -R "$top_dir/src/native_plugins/$solver/maude_se_native_$solver" "$stage/"
package_dir="$stage/maude_se_native_$solver"
mkdir -p "$package_dir/licenses"

case "$solver" in
  z3)
    cp "$top_dir/.build-standalone/dependencies/z3-$Z3_VERSION/LICENSE.txt" "$package_dir/licenses/Z3-LICENSE.txt"
    ;;
  yices)
    cp "$top_dir/.build-standalone/dependencies/yices-$(uname -s)-$(uname -m)/yices-$YICES_VERSION/LICENSE" "$package_dir/licenses/YICES-LICENSE"
    cp "$top_dir/.build-standalone/dependencies/yices-$(uname -s)-$(uname -m)/yices-$YICES_VERSION/NOTICES" "$package_dir/licenses/YICES-NOTICES"
    cp "$top_dir/.build-standalone/dependencies/cudd-3.0.0/LICENSE" "$package_dir/licenses/CUDD-LICENSE"
    ;;
  cvc5)
    platform="$(uname -s)"
    [[ "$platform" == Darwin ]] && platform=macOS
    bundle="$top_dir/.build-standalone/dependencies/cvc5-$platform-$(uname -m)-static"
    cp "$bundle/COPYING" "$package_dir/licenses/CVC5-COPYING"
    cp "$bundle/licenses/"* "$package_dir/licenses/"
    ;;
esac

includes=(-I"$release_dir" -I"$python_include" -I"$native_prefix/include" -I"$top_dir/.build-wheel/install/include" \
  -I"$top_dir/.build-wheel/sources/maude-bindings/src")
for directory in "$source_dir"/src/*; do
  [[ -d "$directory" ]] && includes+=(-I"$directory")
done

output="$package_dir/libmaude_se_$solver.$suffix"
"${CXX:-c++}" -std=c++17 -O2 -fPIC -DHAVE_CONFIG_H -DUSE_PYSMT \
  "-D$plugin_define" "${includes[@]}" \
  "${link_mode[@]}" \
  "$top_dir/src/native_plugins/plugin.cc" \
  "$top_dir/src/Extension/$source_file" \
  -L"$release_dir" -lmaude "${archive_paths[@]}" \
  -o "$output"

mkdir -p "$top_dir/out"
MACOSX_DEPLOYMENT_TARGET="$target" "$build_python" -m pip wheel \
  --no-deps --no-build-isolation -w "$top_dir/out" "$stage"
