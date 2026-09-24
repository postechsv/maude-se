#!/usr/bin/env bash

# Settings/Utilities
# ------------------

# failed on any error
set -euo pipefail

# set variables
top_dir="${MAUDE_SE_TOP_DIR:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"

# shellcheck source=versions.env
source "$top_dir/build/versions.env"
# shellcheck source=source-integrity.sh
source "$top_dir/build/source-integrity.sh"

# shellcheck source=version.sh
source "$top_dir/build/version.sh"

work_dir="$top_dir/.build-standalone"
maude_dir="$work_dir/sources/Maude"
build_dir="$work_dir/install"
third_party="$work_dir/dependencies"

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
  native_cflags="-O3 -fno-stack-protector -mmacosx-version-min=$deployment_target"
  native_cxxflags="$native_cflags -std=c++17"
  native_ldflags="-mmacosx-version-min=$deployment_target"
else
  deployment_target=""
  native_cflags="-O3 -fno-stack-protector"
  native_cxxflags="$native_cflags -std=c++17"
  native_ldflags=""
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
  local marker="$work_dir/patches/$(basename "$patch_file").applied"
  local signature

  signature="$(git -C "$dir" rev-parse HEAD):$(git hash-object "$patch_file")"
  if [[ -f "$marker" && "$(<"$marker")" == "$signature" ]]; then
    progress "Patch already applied: $(basename "$patch_file")"
    return 0
  fi

  if git -C "$dir" apply -p0 --check "$patch_file" 2>/dev/null; then
    git -C "$dir" apply -p0 "$patch_file"
  elif git -C "$dir" apply -p0 --reverse --check "$patch_file" 2>/dev/null; then
    progress "Patch already applied: $(basename "$patch_file")"
  else
    echo "error: patch does not apply cleanly: $patch_file" >&2
    return 1
  fi
  mkdir -p "$(dirname "$marker")"
  printf '%s\n' "$signature" >"$marker"
}

setup_build() {
  progress "Setup build essentials"

  # for GitHub CI/CD
  if [[ "$os" == "Darwin" ]]; then
    brew install bison flex autoconf automake cmake

    export PATH="$(brew --prefix bison)/bin:$PATH"
    export PATH="$(brew --prefix flex)/bin:$PATH"
    export PATH="$(brew --prefix autoconf)/bin:$PATH"
    export PATH="$(brew --prefix automake)/bin:$PATH"
  else
    yum install flex bison zip unzip -y
  fi
}

build_deps() {
  local solver="${1:-z3}"

  build_libsigsegv
  build_gmp
  build_buddy
  build_tecla
  build_ncurses

  case "$solver" in
    z3) build_z3 ;;
    yices) get_yices; get_cvc5; build_cudd ;;
    cvc5) get_cvc5 ;;
    *) echo "error: unsupported native standalone solver: $solver" >&2; return 2 ;;
  esac

  rm -rf "$build_dir"/lib/*.so*
  rm -rf "$build_dir"/lib/*.dylib*
}

get_cvc5() {
  local package_name expected archive source_dir
  case "$os:$arch" in
    Darwin:arm64)
      package_name="cvc5-macOS-arm64-static"
      expected="$CVC5_MACOS_ARM64_SHA256" ;;
    Darwin:x86_64)
      package_name="cvc5-macOS-x86_64-static"
      expected="$CVC5_MACOS_X86_64_SHA256" ;;
    Linux:x86_64)
      package_name="cvc5-Linux-x86_64-static"
      expected="$CVC5_LINUX_X86_64_SHA256" ;;
    *) echo "error: cvc5 has no pinned static package for $os/$arch" >&2; return 1 ;;
  esac
  mkdir -p "$third_party" "$include_dir" "$lib_dir"
  archive="$third_party/$package_name.zip"
  source_dir="$third_party/$package_name"
  if [[ ! -f "$archive" ]]; then
    curl -fLsS --retry 3 -o "$archive" \
      "https://github.com/cvc5/cvc5/releases/download/cvc5-$CVC5_VERSION/$package_name.zip"
  fi
  verify_source_archive "$archive" "$expected"
  if [[ ! -d "$source_dir" ]]; then
    unzip -q "$archive" -d "$third_party"
  fi
  cp -R "$source_dir/include/cvc5" "$include_dir/"
  cp "$source_dir/lib/libcvc5.a" "$source_dir/lib/libpicpolyxx.a" \
    "$source_dir/lib/libpicpoly.a" "$source_dir/lib/libcadical.a" \
    "$source_dir/lib/libmpfr.a" "$lib_dir/"
}

prepare() {
  ensure_repo "https://github.com/maude-lang/Maude.git" "$maude_dir" "$MAUDE_REF"
  patch_maude
}

patch_maude() {
  progress "Apply patching"

  apply_patch_once "$maude_dir" "$top_dir/src/patch/$MAUDE_NATIVE_PATCH"
  apply_patch_once "$maude_dir" "$top_dir/src/patch/native-cvc5.patch"
}

make_patch() {
  progress "Make patch for Maude as a library"

  cd "$maude_dir"
  git diff --no-prefix >$top_dir/src/patch/e-$(git log -1 --pretty=format:"%h").patch
}

build_all() {
  local release_tag="${1:-$(maude_se_release_tag)}"
  local solver="${2:-z3}"

  validate_maude_se_release_tag "$release_tag"
  setup_build
  prepare
  build_deps "$solver"
  build_maude_se "$release_tag" "$solver"
}

build_maude_se() {
  local release_tag="$1"
  local solver="${2:-z3}"
  case "$solver" in
    z3) build_maude z3 "--with-yices2=no --with-cvc4=no --with-cvc5=no --with-z3=yes" "-pthread" "$release_tag" ;;
    yices) build_maude yices "--with-yices2=yes --with-cvc4=no --with-cvc5=no --with-z3=no" "-pthread" "$release_tag" ;;
    cvc5) build_maude cvc5 "--with-yices2=no --with-cvc4=no --with-cvc5=yes --with-z3=no" "-pthread" "$release_tag" ;;
    *) echo "error: unsupported native standalone solver: $solver" >&2; return 2 ;;
  esac
}

build_maude() {
  local name="$1"          # First argument: build name or tag
  local config_opts="$2"   # Second argument: configure option bundle (string)
  local extra_ldflags="$3" # Third argument: additional LDFLAGS (string)
  local version="${4#v}"
  local package_src_dir="$build_dir/package-src"

  progress "Build MaudeSE ($name)"
  prepare_maude_se_package_sources "$package_src_dir"
  rm -rf "$maude_dir/src/Extension"
  cp -r "$top_dir/src/Extension" "$maude_dir/src"

  if [[ "$os" == "Darwin" ]]; then
    os_n="macosx"
    if [[ "$arch" == "x86_64" ]]; then
      arch_n="x86_64"
    else
      arch_n="arm64"
    fi
  else
    os_n="manylinux"
    arch_n="x86_64"
  fi

  local out_name="maude_se_$name-$version-$os_n-$arch_n"

  cd "$maude_dir"
  autoreconf -i

  rm -rf "$maude_dir/Out"
  mkdir -p "$maude_dir/Out"
  cd "$maude_dir/Out"

  CXXFLAGS="$native_cxxflags -Wall"
  LDFLAGS="-L$lib_dir $native_ldflags $extra_ldflags"
  if [[ "$os" == "Linux" ]]; then
    CXXFLAGS+=" -static-libstdc++ -static-libgcc"
    LDFLAGS+=" -static-libstdc++ -static-libgcc"
  fi

  ../configure \
    $config_opts \
    CPPFLAGS="-I$include_dir" \
    CXXFLAGS="$CXXFLAGS" \
    LDFLAGS="$LDFLAGS" \
    TECLA_LIBS="$lib_dir/libtecla.a $lib_dir/libncursesw.a" \
    GMP_LIBS="$lib_dir/libgmpxx.a $lib_dir/libgmp.a" \
    Z3_LIB="$lib_dir/libz3.a" \
    YICES2_LIB="$lib_dir/libyices.a $lib_dir/libpicpoly.a $lib_dir/libcudd.a $lib_dir/libgmp.a" \
    CVC5_LIB="$lib_dir/libcvc5.a $lib_dir/libpicpolyxx.a $lib_dir/libpicpoly.a $lib_dir/libcadical.a $lib_dir/libmpfr.a $lib_dir/libgmpxx.a $lib_dir/libgmp.a"

  make -j4
  make check
  cd src/Main
  mkdir -p "$out_name"
  strip maude
  cp maude "$out_name/maude-se-$name"
  cp $maude_dir/src/Main/*.maude ./"$out_name"
  cp "$package_src_dir"/*.maude ./"$out_name"
  if [[ "$name" == cvc5 ]]; then
    local cvc5_platform="$os"
    [[ "$os" == Darwin ]] && cvc5_platform=macOS
    cp "$third_party/cvc5-$cvc5_platform-$arch-static/COPYING" "$out_name/CVC5-COPYING"
    cp -R "$third_party/cvc5-$cvc5_platform-$arch-static/licenses" "$out_name/licenses"
  fi
  if [[ "$name" == yices ]]; then
    cp "$third_party/yices-$os-$arch/yices-$YICES_VERSION/LICENSE" "$out_name/YICES-LICENSE"
    cp "$third_party/yices-$os-$arch/yices-$YICES_VERSION/NOTICES" "$out_name/YICES-NOTICES"
    cp "$third_party/cudd-3.0.0/LICENSE" "$out_name/CUDD-LICENSE"
    local cvc5_platform="$os"
    [[ "$os" == Darwin ]] && cvc5_platform=macOS
    cp "$third_party/cvc5-$cvc5_platform-$arch-static/COPYING" "$out_name/CVC5-COPYING"
    cp -R "$third_party/cvc5-$cvc5_platform-$arch-static/licenses" "$out_name/licenses"
  fi

  zip -r "$out_name.zip" "$out_name"

  mkdir -p "$top_dir/out"
  mv "$out_name.zip" "$top_dir/out"
}

# build gmp
build_gmp() {
  progress "Building gmp"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"

  local gmp_cpu_opts=(--enable-fat)
  if [[ "$os" == "Darwin" && "$arch" == "x86_64" ]]; then
    # GMP's Intel assembly contains text relocations rejected by modern macOS
    # linkers, even in a non-fat build. Use portable C for static linking.
    gmp_cpu_opts=(--disable-fat --disable-assembly)
  fi

  progress "Downloading gmp $GMP_VERSION"
  rm -rf "$third_party/gmp-$GMP_VERSION"
  get_gnu "gmp" "$GMP_VERSION" "tar.xz"
  cd "$third_party/gmp-$GMP_VERSION"

  ./configure --prefix="$build_dir" \
    CFLAGS="$native_cflags" CXXFLAGS="$native_cxxflags" LDFLAGS="$native_ldflags" \
    --enable-cxx "${gmp_cpu_opts[@]}" --disable-shared --enable-static

  make -j4
  make install
}

build_buddy() {
  progress "Building BuDDy library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  (
    progress "Downloading BuDDy $BUDDY_VERSION"
    buddy_dir="$third_party/buddy-$BUDDY_VERSION"
    rm -rf "$buddy_dir"

    download_source_archive \
      "https://github.com/utwente-fmt/buddy/releases/download/v$BUDDY_VERSION/buddy-$BUDDY_VERSION.tar.gz" \
      "$buddy_dir.tar.gz" "buddy-$BUDDY_VERSION"
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
    chmod a+x ./tools/install-sh
    make install
  )
}

build_tecla() {
  progress "Build libtecla"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  progress "Downloading Tecla $TECLA_VERSION"
  tecla_dir="$third_party/libtecla"
  rm -rf "$tecla_dir"

  download_source_archive \
    "https://deb.debian.org/debian/pool/main/libt/libtecla/libtecla_$TECLA_VERSION.orig.tar.gz" \
    "$tecla_dir.tar.gz" "libtecla-$TECLA_VERSION"
  tar -xzf "$tecla_dir.tar.gz" -C "$third_party"
  rm -f "$tecla_dir.tar.gz"

  cd "$tecla_dir"
  # Tecla 1.6.3 predates modern Darwin version numbers. Use the pinned,
  # up-to-date GNU platform detection scripts shipped with this project.
  cp "$top_dir/build/config.guess" "$top_dir/build/config.sub" ./
  chmod +x config.guess config.sub

  if [[ "$os" == "Darwin" ]] && ! grep -Fq '#include <sys/ioctl.h>' enhance.c; then
    sed -i.bak '1i\
#include <sys/ioctl.h>\
' enhance.c
    rm -f enhance.c.bak
  fi
  if [[ "$os" == "Darwin" ]]; then
    # Tecla's old macOS special case declares the tputs callback as void.
    # Current macOS SDKs and ncurses use int (*)(int), like other platforms.
    sed -i.bak \
      's/#elif defined(__APPLE__) && defined(__MACH__)/#elif defined(__APPLE__) \&\& defined(__MACH__) \&\& defined(TECLA_TPUTS_RETURNS_VOID)/' \
      getline.c
    rm -f getline.c.bak
  fi

  ./configure CFLAGS="$native_cflags" LDFLAGS="$native_ldflags" \
    --prefix="$build_dir"
  # Tecla's generated Makefile does not order normal_obj before its file targets.
  make TARGETS=normal TARGET_LIBS=static DEMOS= PROGRAMS=
  make install_inc
  install -m 644 libtecla.a "$lib_dir/libtecla.a"
}

build_ncurses() {
  progress "Build libncurses"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  progress "Downloading ncurses $NCURSES_VERSION"
  rm -rf "$third_party/ncurses-$NCURSES_VERSION"
  get_gnu "ncurses" "$NCURSES_VERSION" "tar.gz"
  libncurses_dir="$third_party/ncurses-$NCURSES_VERSION"

  cd "$libncurses_dir"
  ./configure --with-normal --with-static --without-shared --without-debug \
    --enable-widec --prefix="$build_dir" CFLAGS="$native_cflags" \
    CXXFLAGS="$native_cxxflags" LDFLAGS="$native_ldflags"

  make -j4
  make install
}

build_libsigsegv() {
  progress "Build libsigsegv"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"

  progress "Downloading libsigsegv $LIBSIGSEGV_VERSION"
  rm -rf "$third_party/libsigsegv-$LIBSIGSEGV_VERSION"
  get_gnu "libsigsegv" "$LIBSIGSEGV_VERSION" "tar.gz"
  sigsegv_dir="$third_party/libsigsegv-$LIBSIGSEGV_VERSION"

  cd "$sigsegv_dir"
  ./configure CFLAGS="$native_cflags" LDFLAGS="$native_ldflags" \
    --prefix="$build_dir" --enable-shared=no

  make -j4
  make install
}

build_z3() {
  progress "Building z3"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  (
    local z3_dir="$third_party/z3-$Z3_VERSION"
    progress "Downloading Z3 $Z3_VERSION"
    rm -rf "$z3_dir"
    git clone --branch "z3-$Z3_VERSION" --depth 1 \
      https://github.com/Z3Prover/z3 "$z3_dir"
    [[ "$(git -C "$z3_dir" rev-parse HEAD)" == "$Z3_4_13_0_REF" ]] || {
      echo "error: Z3 tag z3-$Z3_VERSION does not resolve to the pinned commit" >&2
      return 1
    }

    # Z3 4.13.0 contains a stale accessor name that newer Clang versions
    # instantiate and reject while compiling static_matrix::ref.
    sed -i.bak \
      's/v\.m_matrix\.get(v\.m_row, v\.m_col)/v.m_matrix.get_elem(v.m_row, v.m_col)/' \
      "$z3_dir/src/math/lp/static_matrix.h"
    rm -f "$z3_dir/src/math/lp/static_matrix.h.bak"
    sed -i.bak 's/c\.m_low_bound/c.m_lower_bound/' \
      "$z3_dir/src/math/lp/column_info.h"
    rm -f "$z3_dir/src/math/lp/column_info.h.bak"
    sed -i.bak 's/A\.get_value_of_column_cell(col)/A.get_val(col)/' \
      "$z3_dir/src/math/lp/static_matrix_def.h"
    rm -f "$z3_dir/src/math/lp/static_matrix_def.h.bak"

    cmake -S "$z3_dir" -B "$z3_dir/build" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$build_dir" \
      -DCMAKE_INSTALL_LIBDIR=lib \
      -DCMAKE_OSX_ARCHITECTURES="$arch" \
      -DCMAKE_OSX_DEPLOYMENT_TARGET="$deployment_target" \
      -DCMAKE_C_FLAGS="$native_cflags" \
      -DCMAKE_CXX_FLAGS="$native_cxxflags" \
      -DCMAKE_EXE_LINKER_FLAGS="$native_ldflags" \
      -DCMAKE_SHARED_LINKER_FLAGS="$native_ldflags" \
      -DZ3_BUILD_LIBZ3_SHARED=OFF
    cmake --build "$z3_dir/build" --parallel 4
    cmake --install "$z3_dir/build"
  )

  [[ -f "$lib_dir/libz3.a" ]] || {
    echo "error: Z3 static library was not installed at $lib_dir/libz3.a" >&2
    return 1
  }
  rm -f "$build_dir"/lib/libz3*.so* "$build_dir"/lib/libz3*.dylib
}

get_z3() {
  progress "Get Z3"
  mkdir -p "$third_party"
  mkdir -p "$build_dir"
  mkdir -p "$build_dir/lib"
  mkdir -p "$build_dir/include"

  if [[ "$os" == "Darwin" ]]; then
    z3_os="osx.11.*"
    if [[ "$arch" == "x86_64" ]]; then
      z3_arch="x64"
    else
      z3_arch="arm64"
    fi
  else
    z3_os="glibc.*2.31"
    z3_arch="x64"
  fi

  # get the version that is compatiable with GLIBC 2.31 (Ubuntu 20.04)
  url=$(git_tag "Z3Prover" "z3" "$z3_arch.*$z3_os" "z3-4.13.0")

  z3_tmp="$third_party/z3.zip"
  z3_dir=$(get_smt "$url" "$z3_tmp")

  echo $z3_dir
  rm -rf "$z3_dir"
  unzip "$z3_tmp" -d "$third_party"

  rm -rf "$z3_tmp"

  mv $z3_dir/bin/libz3.a $build_dir/lib
  mv $z3_dir/include/* $build_dir/include
}

get_yices() {
  local package_name expected archive package_dir source_dir
  case "$os:$arch" in
    Darwin:arm64)
      package_name="yices-$YICES_VERSION-arm-apple-darwin22.6.0-static-gmp"
      expected="$YICES_MACOS_ARM64_SHA256" ;;
    Darwin:x86_64)
      package_name="yices-$YICES_VERSION-x86_64-apple-darwin21.6.0-static-gmp"
      expected="$YICES_MACOS_X86_64_SHA256" ;;
    Linux:x86_64)
      package_name="yices-$YICES_VERSION-x86_64-pc-linux-gnu-static-gmp"
      expected="$YICES_LINUX_X86_64_SHA256" ;;
    *) echo "error: Yices has no pinned static package for $os/$arch" >&2; return 1 ;;
  esac
  progress "Install Yices $YICES_VERSION ($os/$arch)"
  mkdir -p "$third_party" "$include_dir" "$lib_dir"
  archive="$third_party/$package_name.tar.gz"
  package_dir="$third_party/yices-$os-$arch"
  source_dir="$package_dir/yices-$YICES_VERSION"
  if [[ ! -f "$archive" ]]; then
    curl -fLsS --retry 3 -o "$archive" \
      "https://github.com/SRI-CSL/yices2/releases/download/Yices-$YICES_VERSION/$package_name.tar.gz"
  fi
  verify_source_archive "$archive" "$expected"
  if [[ ! -d "$source_dir" ]]; then
    mkdir -p "$package_dir"
    tar -xzf "$archive" -C "$package_dir"
  fi
  cp "$source_dir/include"/*.h "$include_dir/"
  cp "$source_dir/lib/libyices.a" "$lib_dir/"
}

build_cudd() {
  local cudd_dir="$third_party/cudd-3.0.0"
  progress "Build CUDD 3.0.0 for Yices"
  ensure_repo "https://github.com/ivmai/cudd.git" "$cudd_dir" "$CUDD_REF"
  (
    cd "$cudd_dir"
    ./configure CFLAGS="$native_cflags" CXXFLAGS="$native_cxxflags" \
      LDFLAGS="$native_ldflags" --prefix="$build_dir" --disable-shared
    make -j4
    make install
  )
}

get_smt() {
  curl -s -L -o "$2" "$1"

  file=$(basename $1)
  smt_dir="$third_party/${file%.*}"

  echo $smt_dir
}

build_from_brew() {

  mkdir -p "$build_dir/include"
  mkdir -p "$build_dir/lib"

  brew install $1

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
  download_source_archive \
    "https://ftp.gnu.org/gnu/$name/$libname.$ext" \
    "$third_party/$libname.$ext" "$libname"
  tar -xf "$third_party/$libname.$ext" -C "$third_party"
  rm -rf "$third_party/$libname.$ext"
}

get_brew_pkg() {
  echo "$(brew --cellar $1)/$(brew list --versions $1 | tr ' ' '\n' | tail -1)"
}

git_latest_name() {
  echo $(github_curl "https://api.github.com/repos/$1/$2/releases/latest" |
    grep '"tag_name":' |
    head -n1 |
    cut -d '"' -f 4)
}

git_latest() {
  latest_release=$(github_curl "https://api.github.com/repos/$1/$2/releases/latest")
  url=$(echo "$latest_release" | grep "browser_download_url" | grep -E "$3" | cut -d '"' -f 4)

  if [[ -z "$url" ]]; then
    echo "Error: nothing found in the latest release of $1/$2"
    exit 1
  fi

  echo $url
}

git_tag() {
  release=$(github_curl "https://api.github.com/repos/$1/$2/releases/tags/$4")
  url=$(echo "$release" | grep "browser_download_url" | grep -E "$3" | cut -d '"' -f 4)

  if [[ -z "$url" ]]; then
    echo "Error: nothing found in the latest release of $1/$2"
    exit 1
  fi

  echo $url
}

github_curl() {
  local url="$1"

  if [[ -n "${GITHUB_TOKEN:-}" ]]; then
    curl -sS -H "Authorization: Bearer $GITHUB_TOKEN" "$url"
  else
    curl -sS "$url"
  fi
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
#  3. patch_src
#  4. build_maude_lib
#  5. build_maude_se

# Main
# ----

build_command="${1:-help}"
if [[ $# -gt 0 ]]; then
  shift
fi
case "$build_command" in
set-env) setup_build "$@" ;;
prep) prepare "$@" ;;
deps) build_deps "$@" ;;
build-maude-se) build_maude_se "$@" ;;
build) build_all "$@" ;;
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
