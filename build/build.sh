#!/usr/bin/env bash

# Settings/Utilities
# ------------------

# error if an variable is referenced before being set
set -u

# set variables
top_dir="$(pwd)"
src_dir="$(pwd)/src"

# maudesmc
smc_dir="$top_dir/maude-bindings/subprojects/maudesmc"

build_dir="$top_dir/.build"
third_party="$top_dir/.3rd_party"

# OS & architecture detection

os="$(uname -s)"
arch="$(uname -m)"

# -------------------
include_dir="$build_dir/include"
lib_dir="$build_dir/lib"

# functionality
progress() { echo "===== " $@; }

prepare() {
  pip install meson scikit-build ninja cmake swig build

  git clone https://github.com/fadoss/maude-bindings.git
  cd maude-bindings && git submodule update --init
  patch_maude
}

patch_maude() {
  progress "Apply patchings"

  cd "$top_dir/maude-bindings"
  patch -p0 <$top_dir/src/patch/b-*.patch

  cd "$smc_dir"
  patch -p0 <$top_dir/src/patch/c-*.patch
  patch -p0 <$top_dir/src/patch/d-*.patch
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
    $arch_opt meson setup release -Dcpp_args="-fno-stack-protector -fstrict-aliasing" \
      -Dextra-lib-dirs="$build_dir/lib" \
      -Dextra-include-dirs="$build_dir/include, $py_inc, $top_dir/maude-bindings/src" \
      -Dstatic-libs='buddy, gmp, sigsegv' \
      -Dwith-smt='pysmt' \
      -Dwith-ltsmin=disabled \
      -Dwith-simaude=disabled \
      -Dc_args='-mno-thumb' \
      -Dc_link_args="-Wl,--export-dynamic" \
      -Dcpp_link_args="-Wl,-x -u $undef_symb -L"$build_dir"/lib -lgmp" \
      -Dcpp_std=c++17
    cd release && ninja
  )
}

prep_build_maude_se() {
  swig_src_dir="$top_dir/maude-bindings/swig"

  mkdir -p $smc_dir/build
  mkdir -p $smc_dir/installdir/lib

  cp $smc_dir/release/config.h $smc_dir/build
  cp $smc_dir/release/libmaude.so $smc_dir/installdir/lib
  cp $smc_dir/release/libmaude.dylib $smc_dir/installdir/lib

  strip $smc_dir/installdir/lib/*.so # only for Linux

  cp "$top_dir/src/pyproject.toml" $top_dir/maude-bindings
  cp "$top_dir/README.md" $top_dir/maude-bindings

  cp "$src_dir/swig/rwsmt.i" "$swig_src_dir"
  cp "$src_dir/swig/core.i" "$swig_src_dir"
  cp "$src_dir/Extension/pysmt.hh" "$top_dir/maude-bindings/src"
}

build_maude_se() {
  prep_build_maude_se

  cd maude-bindings
  (
    rm -rf dist/ maude.egg-info/ _skbuild/
    CMAKE_ARGS="-DBUILD_LIBMAUDE=OFF -DEXTRA_INCLUDE_DIRS=$build_dir/include -DMAUDE_SE_INSTALL_FILES=$top_dir/src" \
    ARCHFLAGS="-arch $arch" \
      python -m pip wheel -w dist --no-deps .
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
    build_from_brew "gmp"
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

    curl -L https://github.com/utwente-fmt/buddy/releases/download/v2.4/buddy-2.4.tar.gz >"$buddy_dir.tar.gz"
    tar -xvzf "$buddy_dir.tar.gz" -C "$third_party"
    rm -rf "$buddy_dir.tar.gz"

    cd "$buddy_dir"

    # replace old config guess to latest one
    rm -rf config.guess config.sub

    # this is unstable
    # wget -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
    # wget -O config.sub 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'

    cp $top_dir/build/config.sub $top_dir/build/config.guess ./

    ./configure CFLAGS="-fPIC" CXXFLAGS="-fPIC" --includedir="$include_dir" --libdir="$lib_dir" --disable-shared

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
    build_from_brew "libtecla"
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
    build_from_brew "libsigsegv"
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

  brew install $1

  brew_dir=$(get_brew_pkg $1)

  # copy
  cp $brew_dir/include/* $build_dir/include
  cp $brew_dir/lib/* $build_dir/lib

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

# Follow the below steps
#  1. prepare
#  2. build_deps
#  3. patch_maude
#  4. build_maude_lib
#  5. build_maude_se

# Main
# ----

build_command="$1"
shift
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
