#!/usr/bin/env bash
set -euo pipefail

top_dir="${MAUDE_SE_TOP_DIR:-$(cd "$(dirname "$0")/.." && pwd)}"
if [[ -n "${MAUDE_SE_WHEEL_WORK_DIR:-}" ]]; then
  work_dir="$MAUDE_SE_WHEEL_WORK_DIR"
else
  if [[ "$(uname -s)" == Darwin ]]; then
    python_tag="$(python3 -c 'import sys; print(f"cp{sys.version_info.major}{sys.version_info.minor}")')"
    work_dir="$top_dir/.build-wheel/$python_tag-$(uname -m)"
  else
    work_dir="$top_dir/.build-wheel"
  fi
fi
if [[ -n "${MAUDE_SE_WHEEL_DEPS_DIR:-}" ]]; then
  wheel_deps_dir="$MAUDE_SE_WHEEL_DEPS_DIR"
elif [[ "$(uname -s)" == Darwin ]]; then
  wheel_deps_dir="$top_dir/.build-wheel"
else
  wheel_deps_dir="$work_dir"
fi
solver="${1:-}"
case "$solver" in z3 | yices | cvc5) ;; *) echo "usage: $0 z3|yices|cvc5" >&2; exit 2 ;; esac
output="${MAUDE_SE_PLUGIN_OUTPUT:-}"
[[ -n "$output" ]] || { echo "error: MAUDE_SE_PLUGIN_OUTPUT is required; use maude-se-installer install native $solver" >&2; exit 2; }
[[ -n "${MAUDE_SE_CORE_LIBRARY:-}" ]] || { echo "error: MAUDE_SE_CORE_LIBRARY is required" >&2; exit 2; }
[[ -n "${MAUDE_SE_PLUGIN_ASSET_DIR:-}" ]] || { echo "error: MAUDE_SE_PLUGIN_ASSET_DIR is required" >&2; exit 2; }

source_dir="$work_dir/sources/maude-bindings/subprojects/maudesmc"
release_dir="$source_dir/release"
native_prefix="${MAUDE_SE_NATIVE_PREFIX:-$top_dir/.build-standalone/install}"
build_python="$work_dir/venv-build/bin/python"
[[ -x "$build_python" ]] || { echo "error: build virtualenv is missing" >&2; exit 1; }
python_include="$($build_python -c 'import sysconfig; print(sysconfig.get_path("include"))')"
if [[ "$(uname -s)" == Darwin ]]; then
  suffix=dylib
  core_library="$MAUDE_SE_CORE_LIBRARY"
  core_rpath=@loader_path/../../maude
  link_mode=(-dynamiclib "-Wl,-install_name,@rpath/libmaude_se_$solver.dylib" \
    "-Wl,-rpath,$core_rpath" -Wl,-rpath,@loader_path/solver)
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
  core_library="$MAUDE_SE_CORE_LIBRARY"
  core_rpath='$ORIGIN/../../maude'
  link_mode=(-shared "-Wl,-rpath,$core_rpath" '-Wl,-rpath,$ORIGIN/solver' \
    -Wl,--disable-new-dtags)
  target=""
fi
[[ -f "$core_library" ]] || { echo "error: build the base wheel first: $core_library" >&2; exit 1; }

case "$solver" in
  z3)
    source_file=z3.cc
    plugin_define=MAUDE_SE_PLUGIN_Z3
    ;;
  yices)
    source_file=yices2.cc
    plugin_define=MAUDE_SE_PLUGIN_YICES
    ;;
  cvc5)
    source_file=cvc5.cc
    plugin_define=MAUDE_SE_PLUGIN_CVC5
    ;;
esac
asset_dir="$MAUDE_SE_PLUGIN_ASSET_DIR"
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

mkdir -p "$(dirname "$output")"

includes=(-I"$release_dir" -I"$python_include" -I"$native_prefix/include" -I"$wheel_deps_dir/install/include" \
  -I"$work_dir/sources/maude-bindings/src")
for directory in "$source_dir"/src/*; do
  [[ -d "$directory" ]] && includes+=(-I"$directory")
done

"${CXX:-c++}" -std=c++17 -O2 -fPIC -DHAVE_CONFIG_H -DUSE_PYSMT \
  "-D$plugin_define" "${includes[@]}" \
  "${link_mode[@]}" \
  "$top_dir/src/native_plugins/plugin.cc" \
  "$top_dir/src/Extension/$source_file" \
  -L"$(dirname "$core_library")" -lmaude "${solver_links[@]}" \
  -o "$output"

if [[ "$(uname -s)" == Darwin && "$solver" == z3 ]]; then
  install_name_tool -change libz3.dylib @rpath/libz3.dylib "$output"
elif [[ "$(uname -s)" == Darwin && "$solver" == yices ]]; then
  yices_install_name="$(otool -D "$asset_dir/libyices.2.dylib" | tail -n 1)"
  install_name_tool -change "$yices_install_name" @rpath/libyices.2.dylib "$output"
fi
