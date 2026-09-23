#!/usr/bin/env bash

# Settings/Utilities
# ------------------

# failed on any error
set -euo pipefail

# set variables
top_dir="${MAUDE_SE_TOP_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
src_dir="$top_dir/src"

# shellcheck source=versions.env
source "$top_dir/build/versions.env"

# shellcheck source=version.sh
source "$top_dir/build/version.sh"

# maudesmc
smc_dir="$top_dir/maude-bindings/subprojects/maudesmc"

build_dir="$top_dir/.build"
third_party="$top_dir/.3rd_party"
package_src_dir="$build_dir/package-src"

# OS & architecture detection

os="$(uname -s)"
arch="$(uname -m)"

if [[ "$os" == "Darwin" ]]; then
  if [[ "$arch" == "arm64" ]]; then
    deployment_target="${MAUDE_SE_MACOS_DEPLOYMENT_TARGET:-11.0}"
  else
    deployment_target="${MAUDE_SE_MACOS_DEPLOYMENT_TARGET:-10.13}"
  fi
  export MACOSX_DEPLOYMENT_TARGET="$deployment_target"
  native_cflags="-O3 -fPIC -fno-stack-protector -mmacosx-version-min=$deployment_target"
  native_cxxflags="$native_cflags -std=c++17"
  native_ldflags="-mmacosx-version-min=$deployment_target"
  native_meson_cflags="$native_cflags"
  if [[ "$arch" == "arm64" ]]; then
    native_meson_cflags+=" -mno-thumb"
  fi
else
  deployment_target=""
  native_cflags="-O3 -fPIC -fno-stack-protector"
  native_cxxflags="$native_cflags -std=c++17"
  native_ldflags=""
  native_meson_cflags="$native_cflags"
fi

# -------------------
include_dir="$build_dir/include"
lib_dir="$build_dir/lib"

# functionality
progress() { echo "===== " $@; }

ensure_repo() {
  local url="$1"
  local dir="$2"
  local ref="$3"
  local current_ref

  if [[ ! -e "$dir/.git" ]]; then
    git clone "$url" "$dir"
  fi

  current_ref="$(git -C "$dir" rev-parse HEAD)"
  if [[ "$current_ref" != "$ref" ]]; then
    if [[ -n "$(git -C "$dir" status --porcelain)" ]]; then
      echo "error: $dir has local changes at unexpected revision $current_ref" >&2
      return 1
    fi
    git -C "$dir" checkout --detach "$ref"
  fi
}

apply_patch_once() {
  local dir="$1"
  local patch_file="$2"

  if git -C "$dir" apply -p0 --check "$patch_file" 2>/dev/null; then
    git -C "$dir" apply -p0 "$patch_file"
  elif git -C "$dir" apply -p0 --reverse --check "$patch_file" 2>/dev/null; then
    progress "Patch already applied: $(basename "$patch_file")"
  else
    echo "error: patch does not apply cleanly: $patch_file" >&2
    return 1
  fi
}

prepare() {
  ensure_repo \
    "https://github.com/fadoss/maude-bindings.git" \
    "$top_dir/maude-bindings" \
    "$MAUDE_BINDINGS_REF"
  git -C "$top_dir/maude-bindings" submodule update --init
  ensure_repo \
    "https://github.com/fadoss/maudesmc" \
    "$smc_dir" \
    "$MAUDESMC_REF"
  patch_maude
}

patch_maude() {
  progress "Apply patchings"

  apply_patch_once \
    "$top_dir/maude-bindings" \
    "$top_dir/src/patch/$MAUDE_BINDINGS_PATCH"
  apply_patch_once "$smc_dir" "$top_dir/src/patch/$MAUDESMC_BUILD_PATCH"
  apply_patch_once "$smc_dir" "$top_dir/src/patch/$MAUDESMC_SOURCE_PATCH"
}

make_patch() {
  progress "Make patch for Maude as a library"

  cd "$top_dir/maude-bindings"
  git diff --no-prefix ":^subprojects" ":^pyproject.toml" ":^README.md" >$top_dir/src/patch/b-$(git log -1 --pretty=format:"%h").patch

  cd "$smc_dir"
  git diff --no-prefix ":^src" >$top_dir/src/patch/c-$(git log -1 --pretty=format:"%h").patch
  git diff --no-prefix src/ >$top_dir/src/patch/d-$(git log -1 --pretty=format:"%h").patch
}

build_deps() {

  build_libsigsegv
  build_gmp
  build_buddy
  build_tecla

  rm -rf "$build_dir"/lib/*.so*
  rm -rf "$build_dir"/lib/*.dylib*
}

build_maude() {
  rm -rf "$smc_dir/src/Extension"
  cp -r "$src_dir/Extension" "$smc_dir/src"
  py_inc="$(python -c "from sysconfig import get_paths; print(get_paths()['include'])")"

  if [[ "$os" == "Darwin" ]]; then
    arch_opt="arch -$arch"
    undef_symb="___gmpz_get_d"
  else
    arch_opt=""
    undef_symb="__gmpz_get_d"
  fi

  cd $smc_dir
  (
    rm -rf release
    $arch_opt meson setup release --buildtype=release \
      -Db_lto=true \
      -Dstrip=true \
      -Dcpp_args="$native_cxxflags -fstrict-aliasing" \
      -Dextra-lib-dirs="$build_dir/lib" \
      -Dextra-include-dirs="$build_dir/include, $py_inc, $top_dir/maude-bindings/src" \
      -Dstatic-libs='buddy, gmp, sigsegv' \
      -Dwith-smt='pysmt' \
      -Dwith-ltsmin=disabled \
      -Dwith-simaude=disabled \
      -Dc_args="$native_meson_cflags" \
      -Dc_link_args="-Wl,--export-dynamic" \
      -Dcpp_link_args="$native_ldflags -Wl,-x -u $undef_symb -L"$build_dir"/lib -lgmp" \
      -Dcpp_std=c++17
    cd release && ninja
  )
}

prep_build_maude_se() {
  swig_src_dir="$top_dir/maude-bindings/swig"

  prepare_maude_se_package_sources "$package_src_dir"

  mkdir -p $smc_dir/build
  mkdir -p $smc_dir/installdir/lib

  cp $smc_dir/release/config.h $smc_dir/build

  if [[ "$os" == "Darwin" ]]; then
    cp $smc_dir/release/libmaude.dylib $smc_dir/installdir/lib
    strip -x "$smc_dir/installdir/lib/libmaude.dylib"
  else
    cp $smc_dir/release/libmaude.so $smc_dir/installdir/lib
    strip $smc_dir/installdir/lib/*.so # only for Linux
  fi

  cp "$top_dir/src/pyproject.toml" $top_dir/maude-bindings
  cp "$top_dir/README.md" $top_dir/maude-bindings

  cp "$src_dir/swig/rwsmt.i" "$swig_src_dir"
  cp "$src_dir/swig/core.i" "$swig_src_dir"
  cp "$src_dir/Extension/pysmt.hh" "$top_dir/maude-bindings/src"
}

build_maude_se() {
  local cmake_args

  prep_build_maude_se

  cmake_args="-DBUILD_LIBMAUDE=OFF -DEXTRA_INCLUDE_DIRS=$build_dir/include -DMAUDE_SE_INSTALL_FILES=$package_src_dir"
  if [[ "$os" == "Darwin" ]]; then
    cmake_args+=" -DCMAKE_OSX_DEPLOYMENT_TARGET=$deployment_target"
  fi

  cd maude-bindings
  (
    rm -rf dist/ maude.egg-info/ _skbuild/
    CMAKE_ARGS="$cmake_args" \
      ARCHFLAGS="-arch $arch" \
      python -m pip wheel -w dist --no-deps --no-build-isolation .
  )

  cd "$top_dir"
  mkdir -p ./out
  cp ./maude-bindings/dist/* ./out
}

# build gmp
build_gmp() {
  progress "Building gmp"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"

  if [[ "$os" == "Darwin" ]]; then
    progress "Downloading gmp $GMP_VERSION"
    rm -rf "$third_party/gmp-$GMP_VERSION"
    get_gnu "gmp" "$GMP_VERSION" "tar.xz"
    cd "$third_party/gmp-$GMP_VERSION"
    ./configure --prefix="$build_dir" \
      CFLAGS="$native_cflags" CXXFLAGS="$native_cxxflags" \
      LDFLAGS="$native_ldflags" \
      --enable-cxx --enable-fat --disable-shared --enable-static
    make -j4
    make install
  else
    progress "Downloading gmp 6.1.2"
    get_gnu "gmp" "6.1.2" "tar.xz"
    cd "$third_party/gmp-6.1.2"

    ./configure --prefix="$build_dir" CFLAGS=-fPIC CXXFLAGS=-fPIC \
      --enable-cxx --enable-fat --disable-shared --enable-static --build=x86_64-pc-linux-gnu

    make -j4
    make check
    make install
  fi
}

build_buddy() {
  progress "Building BuDDy library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  (
    progress "Downloading BuDDy 2.4"
    buddy_dir="$third_party/buddy-2.4"
    rm -rf "$buddy_dir"

    curl -fL https://github.com/utwente-fmt/buddy/releases/download/v2.4/buddy-2.4.tar.gz >"$buddy_dir.tar.gz"
    tar -xzf "$buddy_dir.tar.gz" -C "$third_party"
    rm -rf "$buddy_dir.tar.gz"

    cd "$buddy_dir"

    # replace old config guess to latest one
    rm -rf config.guess config.sub

    # this is unstable
    # wget -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
    # wget -O config.sub 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'

    cp $top_dir/build/config.sub $top_dir/build/config.guess ./

    ./configure CFLAGS="$native_cflags" CXXFLAGS="$native_cxxflags" \
      LDFLAGS="$native_ldflags" --includedir="$include_dir" \
      --libdir="$lib_dir" --disable-shared

    make -j4
    make check
    chmod a+x ./tools/install-sh
    make install
  )
}

# build tecla
build_tecla() {
  progress "Build libtecla"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  if [[ "$os" == "Darwin" ]]; then
    progress "Downloading Tecla $TECLA_VERSION"
    tecla_dir="$third_party/libtecla"
    rm -rf "$tecla_dir"
    curl -fL -o "$tecla_dir.tar.gz" \
      "https://sites.astro.caltech.edu/~mcs/tecla/libtecla-$TECLA_VERSION.tar.gz"
    tar -xzf "$tecla_dir.tar.gz" -C "$third_party"
    rm -f "$tecla_dir.tar.gz"
    cd "$tecla_dir"
    cp "$top_dir/build/config.guess" "$top_dir/build/config.sub" ./
    chmod +x config.guess config.sub
    if ! grep -Fq '#include <sys/ioctl.h>' enhance.c; then
      sed -i.bak '1i\
#include <sys/ioctl.h>\
' enhance.c
      rm -f enhance.c.bak
    fi
    sed -i.bak \
      's/#elif defined(__APPLE__) && defined(__MACH__)/#elif defined(__APPLE__) \&\& defined(__MACH__) \&\& defined(TECLA_TPUTS_RETURNS_VOID)/' \
      getline.c
    rm -f getline.c.bak
    ./configure CFLAGS="$native_cflags" LDFLAGS="$native_ldflags" \
      --prefix="$build_dir"
    make -j4 TARGETS=normal TARGET_LIBS=static DEMOS= PROGRAMS=
    make install_inc
    install -m 644 libtecla.a "$lib_dir/libtecla.a"
  else
    progress "Downloading Tecla 1.6.3"
    tecla_dir="$third_party/libtecla"

    curl -o "$tecla_dir.tar.gz" https://sites.astro.caltech.edu/~mcs/tecla/libtecla-1.6.3.tar.gz
    tar -xvzf "$tecla_dir.tar.gz" -C "$third_party"
    rm -rf "$tecla_dir.tar.gz"

    cd "$tecla_dir"

    ./configure CXXFLAGS="-fPIC" CFLAGS="-fPIC -g -fno-stack-protector -O3" \
      --prefix=$build_dir
    make
    make install
  fi
}

# build libsigsegv
build_libsigsegv() {
  progress "Build libsigsegv"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"

  if [[ "$os" == "Darwin" ]]; then
    progress "Downloading libsigsegv $LIBSIGSEGV_VERSION"
    rm -rf "$third_party/libsigsegv-$LIBSIGSEGV_VERSION"
    get_gnu "libsigsegv" "$LIBSIGSEGV_VERSION" "tar.gz"
    sigsegv_dir="$third_party/libsigsegv-$LIBSIGSEGV_VERSION"
    cd "$sigsegv_dir"
    ./configure CFLAGS="$native_cflags" LDFLAGS="$native_ldflags" \
      --prefix="$build_dir" --enable-shared=no
    make -j4
    make install
  else
    progress "Downloading Libsigsegv 2.12"
    get_gnu "libsigsegv" "2.12" "tar.gz"
    sigsegv_dir="$third_party/libsigsegv-2.12"

    cd "$sigsegv_dir"

    ./configure CXXFLAGS="-fPIC" CFLAGS="-fPIC -g -fno-stack-protector -O3" \
      --prefix="$build_dir" --enable-shared=no

    make -j4
    make check
    make install
  fi
}

build_from_brew() {

  mkdir -p "$build_dir/include"
  mkdir -p "$build_dir/lib"

  brew list --versions "$1" >/dev/null 2>&1 || {
    echo "error: missing Homebrew dependency: $1" >&2
    echo "install it with: ./build.sh install-deps" >&2
    return 1
  }

  brew_dir=$(get_brew_pkg $1)

  # copy
  copy_files_only "$brew_dir/include" "$build_dir/include"
  copy_files_only "$brew_dir/lib" "$build_dir/lib"

}

get_brew_pkg() {
  echo "$(brew --cellar $1)/$(brew list --versions $1 | tr ' ' '\n' | tail -1)"
}

get_gnu() {
  name=$1
  version=$2
  ext=$3
  libname="$name-$version"
  mkdir -p "$third_party"
  curl -o "$third_party/$libname.$ext" https://ftp.gnu.org/gnu/$name/$libname.$ext
  tar -xvf "$third_party/$libname.$ext" -C "$third_party"
  rm -rf "$third_party/$libname.$ext"
}

copy_files_only() {
  local src_dir="$1"
  local dst_dir="$2"

  for f in "$src_dir"/*; do
    if [ -f "$f" ]; then
      rm -f "$dst_dir/$(basename "$f")"
      cp "$f" "$dst_dir/"
    fi
  done
}

# Follow the below steps
#  1. prepare
#  2. build_deps
#  3. patch_maude
#  4. build_maude_lib
#  5. build_maude_se

# Main
# ----

build_command="${1:-help}"
if [[ $# -gt 0 ]]; then
  shift
fi
case "$build_command" in
prep) prepare "$@" ;;
deps) build_deps "$@" ;;
patch) patch_maude "$@" ;;
build-maude) build_maude "$@" ;;
build-maude-se) build_maude_se "$@" ;;
prep-build-maude-se) prep_build_maude_se "$@" ;;
make-patch) make_patch "$@" ;;
*) echo "
    usage: $0 [prep|deps|patch|build]
           $0 script <options>

    $0 prep     :   prepare Maude-as-a-library
    $0 deps     :   prepare Maude dependencies
    $0 patch    :   patch Maude-as-a-library
    $0 compile  :   compile Maude-SE
    $0 maude-se :   build Maude-SE
" ;;
esac
