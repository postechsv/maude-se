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
    -Wl,-rpath,@loader_path/../maudeSE/maude -Wl,-rpath,@loader_path/solver)
  if [[ "$solver" == yices ]]; then
    if [[ "$(uname -m)" == arm64 ]]; then
      target=14.0
    else
      target=13.0
    fi
  elif [[ "$(uname -m)" == arm64 ]]; then
    target="${MAUDE_SE_MACOS_DEPLOYMENT_TARGET:-11.0}"
  else
    target="${MAUDE_SE_MACOS_DEPLOYMENT_TARGET:-10.13}"
  fi
  link_mode+=("-mmacosx-version-min=$target")
else
  suffix=so
  core_library="$release_dir/libmaude.so"
  link_mode=(-shared '-Wl,-rpath,$ORIGIN/../maudeSE/maude' '-Wl,-rpath,$ORIGIN/solver' \
    -Wl,--disable-new-dtags)
  target=""
fi
[[ -f "$core_library" ]] || { echo "error: build the base wheel first: $core_library" >&2; exit 1; }

case "$solver" in
  z3)
    source_file=z3.cc
    plugin_define=MAUDE_SE_PLUGIN_Z3
    archives=()
    ;;
  yices)
    source_file=yices2.cc
    plugin_define=MAUDE_SE_PLUGIN_YICES
    archives=()
    ;;
  cvc5)
    source_file=cvc5.cc
    plugin_define=MAUDE_SE_PLUGIN_CVC5
    archives=()
    ;;
esac
archive_paths=()
for archive in "${archives[@]+"${archives[@]}"}"; do
  path="$native_prefix/lib/$archive"
  [[ -f "$path" ]] || { echo "error: missing $path" >&2; exit 1; }
  archive_paths+=("$path")
done

asset_dir="$top_dir/.build-wheel/native-plugin-deps/$solver"
PYTHONPATH="$top_dir/src/pysmt" "$build_python" -c \
  'import native_assets, sys; native_assets.install(sys.argv[1], sys.argv[2], archive=sys.argv[3] or None)' \
  "$solver" "$asset_dir" "${MAUDE_SE_ASSET_ARCHIVE:-}"
if [[ "$solver" == yices ]]; then
  if [[ "$(uname -s)" == Darwin ]]; then
    solver_links=("$asset_dir/libyices.2.dylib")
  else
    solver_links=("$asset_dir/libyices.so.2.6.5")
  fi
else
  solver_links=(-L"$asset_dir" "-l$solver")
fi

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
    cp "$asset_dir/LICENSE" "$package_dir/licenses/YICES-LICENSE"
    ;;
  cvc5)
    cp "$asset_dir/COPYING" "$package_dir/licenses/CVC5-COPYING"
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
  -L"$release_dir" -lmaude "${archive_paths[@]+"${archive_paths[@]}"}" "${solver_links[@]}" \
  -o "$output"

if [[ "$(uname -s)" == Darwin && "$solver" == z3 ]]; then
  install_name_tool -change libz3.dylib @rpath/libz3.dylib "$output"
elif [[ "$(uname -s)" == Darwin && "$solver" == yices ]]; then
  yices_install_name="$(otool -D "$asset_dir/libyices.2.dylib" | tail -n 1)"
  install_name_tool -change "$yices_install_name" @rpath/libyices.2.dylib "$output"
fi

mkdir -p "$top_dir/out"
MACOSX_DEPLOYMENT_TARGET="$target" "$build_python" -m pip wheel \
  --no-deps --no-build-isolation -w "$top_dir/out" "$stage"
