#!/usr/bin/env bash

# Settings/Utilities
# ------------------

# error if an variable is referenced before being set
set -u

# set variables
top_dir="$(pwd)"

maude_dir="$top_dir/Maude"

build_dir="$top_dir/.native-build"
third_party="$top_dir/.native-3rd_party"

# OS & architecture detection

os="$(uname -s)"
arch="$(uname -m)"

# -------------------
include_dir="$build_dir/include"
lib_dir="$build_dir/lib"

# functionality
progress() { echo "===== " $@; }

build_deps() {

  build_libsigsegv
  build_gmp
  build_buddy
  build_tecla
  build_ncurses

  # # get smt solver
  get_z3
  get_yices
  # # get_cvc5

  rm -rf "$build_dir"/lib/*.so*
  rm -rf "$build_dir"/lib/*.dylib*
}

prepare() {
  git clone https://github.com/maude-lang/Maude.git
  cd Maude
  patch_maude
}

patch_maude() {
  progress "Apply patching"

  cd "$top_dir/Maude"
  patch -p0 <$top_dir/src/patch/e-*.patch
}

make_patch() {
  progress "Make patch for Maude as a library"

  cd "$top_dir/Maude"
  git diff --no-prefix >$top_dir/src/patch/e-$(git log -1 --pretty=format:"%h").patch
}

build_maude_se() {
  build_maude "z3" "--with-yices2=no --with-cvc4=no --with-z3=yes" "-pthread"
  build_maude "yices" "--with-yices2=yes --with-cvc4=no --with-z3=no" ""
  # build_maude yices --with-yices2=yes --with-cvc4=no --with-z3=no
  # build_maude cvc4 --with-yices2=no --with-cvc4=yes --with-z3=no
  # Z3_LIB="$lib_dir/libz3.a" \
}

build_maude() {
  local name="$1"              # First argument: build name or tag
  local config_opts="$2"       # Second argument: configure option bundle (string)
  local extra_ldflags="$3"     # Third argument: additional LDFLAGS (string)

  progress "Build MaudeSE ($name)"
  cp -r "$top_dir/src/Extension" "$maude_dir/src"
  local out_name="maude-se-$name"

  cd "$maude_dir"
  autoreconf -i

  mkdir -p "$maude_dir/Out"
  cd "$maude_dir/Out"

  ../configure \
    $config_opts \
    --enable-compiler \
    CPPFLAGS="-I$include_dir" \
    CXXFLAGS="-g -Wall -O3 -fno-stack-protector -static-libstdc++ -static-libgcc" \
    LDFLAGS="-static-libstdc++ -static-libgcc -L$lib_dir $extra_ldflags" \
    TECLA_LIBS="$lib_dir/libtecla.a $lib_dir/libncursesw.a" \
    GMP_LIBS="$lib_dir/libgmpxx.a $lib_dir/libgmp.a"

  make -j4
  make check
  cd src/Main
  mkdir -p "$out_name"
  strip maude
  cp maude "$out_name/$out_name"
  cp $maude_dir/src/Main/*.maude ./"$out_name"
  cp $top_dir/src/*.maude ./"$out_name"
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

    ./configure --prefix="$build_dir" CFLAGS="-O3" CXXFLAGS="-O3" \
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

    ./configure CFLAGS="-fno-stack-protector -O3" CXXFLAGS="-fno-stack-protector -O3" --includedir="$include_dir" --libdir="$lib_dir" --disable-shared

    make -j4
    make check
    chmod a+x ./tools/install-sh
    make install
  )
}

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

    ./configure CFLAGS="-g -fno-stack-protector -O3" \
      --prefix=$build_dir
    make
    make install
  fi
}

build_ncurses() {
  progress "Build libncurses"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  if [[ "$os" == "Darwin" ]]; then
    # build_from_brew "libtecla"
    echo "TODO..."
  else
    progress "Downloading Ncurses 6.1"
    get_gnu "ncurses" "6.1" "tar.gz"
    libncurses_dir="$third_party/ncurses-6.1"

    cd "$libncurses_dir"
    # --enable-widec to support UTF-8
    # libncursesw.a will be generated which does not have dependencies with litinfo
    ./configure --with-normal --with-static --without-shared --without-debug --enable-widec --prefix="$build_dir"

    make -j4
    make install
  fi
}

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
    ./configure CFLAGS="-g -fno-stack-protector -O3" \
      --prefix="$build_dir" --enable-shared=no

    make -j4
    make check
    make install
  fi
}

build_z3() {
  progress "Building z3"
  mkdir -p $build_dir
  mkdir -p "$third_party"
  (
    progress "Downloading Z3"
    git clone https://github.com/Z3Prover/z3 "$third_party/z3"

    cd "$third_party/z3"

    # checkout to the version that is compatiable with GLIBC 2.31 (Ubuntu 20.04)
    git fetch origin refs/tags/z3-4.13.0
    git checkout tags/z3-4.13.0

    python scripts/mk_make.py --prefix="$build_dir" --staticlib CPPFLAGS="-O3 -fno-stack-protector" CXXFLAGS="-O3 -fno-stack-protector"
    cd build
    make -j4
    make install
  )

  rm -rf $build_dir/lib/libz3*.so*
  rm -rf $build_dir/lib/libz3*.dylib
}

get_z3() {
  progress "Get Z3"
  mkdir -p "$third_party"
  mkdir -p "$build_dir"
  mkdir -p "$build_dir/lib"
  mkdir -p "$build_dir/include"

  if [[ "$os" == "Darwin" ]]; then
    z3_os="osx"
    if [[ "$arch" == "x86_64" ]]; then
      z3_arch="x64"
    else
      z3_arch="arm64"
    fi
  else
    z3_os="glibc"
    z3_arch="x64"
  fi

  # get the version that is compatiable with GLIBC 2.31 (Ubuntu 20.04)
  url=$(git_tag "Z3Prover" "z3" "$z3_arch.*$z3_os.*2.31" "z3-4.13.0")

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
  progress "Get Yices"
  mkdir -p "$third_party"
  mkdir -p "$build_dir"
  mkdir -p "$build_dir/lib"
  mkdir -p "$build_dir/include"

  if [[ "$os" == "Darwin" ]]; then
    yices_os="apple"
    if [[ "$arch" == "x86_64" ]]; then
      yices_arch="$arch"
    else
      yices_arch="arm"
    fi
  else
    yices_os="linux"
    yices_arch="$arch"
  fi

  url=$(git_latest "SRI-CSL" "yices2" "$yices_arch.*$yices_os")

  yices_tmp="$third_party/yices.tar.gz"
  yices_dir=$(get_smt "$url" "$yices_tmp")

  yices_dir=$(basename $yices_dir)
  yices_dir="$third_party/${yices_dir%.*}"
  # echo $yices_dir

  rm -rf "$yices_dir"
  tar -xvzf "$yices_tmp" -C "$third_party"

  rm -rf "$yices_tmp"

  yices_dir="$third_party/$(git_latest_name "SRI-CSL" "yices2" "$yices_arch.*$yices_os")"
  yices_dir=${yices_dir,,} # make it lowercase

  mv $yices_dir/lib/libyices.a $build_dir/lib
  mv $yices_dir/include/* $build_dir/include
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

get_brew_pkg() {
  echo "$(brew --cellar $1)/$(brew list --versions $1 | tr ' ' '\n' | tail -1)"
}

git_latest_name() {
  echo $(curl -s -H "Authorization: token $GITHUB_TOKEN" "https://api.github.com/repos/$1/$2/releases/latest" |
    grep '"tag_name":' |
    head -n1 |
    cut -d '"' -f 4)
}

git_latest() {
  latest_release=$(curl -s -H "Authorization: token $GITHUB_TOKEN" "https://api.github.com/repos/$1/$2/releases/latest")
  url=$(echo "$latest_release" | grep "browser_download_url" | grep -E "$3" | cut -d '"' -f 4)

  if [[ -z "$url" ]]; then
    echo "Error: nothing found in the latest release of $1/$2"
    exit 1
  fi

  echo $url
}

git_tag() {
  release=$(curl -s -H "Authorization: token $GITHUB_TOKEN" "https://api.github.com/repos/$1/$2/releases/tags/$4")
  url=$(echo "$release" | grep "browser_download_url" | grep -E "$3" | cut -d '"' -f 4)

  if [[ -z "$url" ]]; then
    echo "Error: nothing found in the latest release of $1/$2"
    exit 1
  fi

  echo $url
}

# Follow the below steps
#  1. prepare
#  2. build_deps
#  3. patch_src
#  4. build_maude_lib
#  5. build_maude_se

# Main
# ----

build_command="$1"
shift
case "$build_command" in
prep) prepare "$@" ;;
deps) build_deps "$@" ;;
build-maude-se) build_maude_se "$@" ;;
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
