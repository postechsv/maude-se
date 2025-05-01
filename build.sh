#!/usr/bin/env bash

# Settings/Utilities
# ------------------

# error if an variable is referenced before being set
set -u

# set variables
top_dir="$(pwd)"
src_dir="$(pwd)/src"

# maudesmc
smc_dir="maude-bindings/subprojects/maudesmc"

maude_raw_dir="$top_dir/test-build/Maude/src"

# build_dir="$top_dir/$smc_dir/.build"
# third_party="$top_dir/$smc_dir/.3rd_party"

# build_dir="$top_dir/test-build/.build"
# third_party="$top_dir/test-build/.3rd_party"

build_dir="$top_dir/.build"
third_party="$top_dir/.3rd_party"

# OS detection
OS=$(uname -s)

# -------------------
include_dir="$build_dir/include"
lib_dir="$build_dir/lib"

# functionality
progress() { echo "===== " $@ ; }

build_deps_workflows() {

  if [ "$OS" == "Darwin" ]; then
    # echo 'export PATH="/opt/homebrew/opt/bison/bin:$PATH"' >> $HOME/.bash_profile
    # echo 'export PATH="/opt/homebrew/opt/flex/bin:$PATH"' >> $HOME/.bash_profile
    # export LDFLAGS="-L/opt/homebrew/opt/bison/lib"
    # export LDFLAGS="-L/opt/homebrew/opt/flex/lib"
    # export CPPFLAGS="-I/opt/homebrew/opt/flex/include"

    brew install libsigsegv
    brew install gmp
    build_buddy
    brew install libtecla

    mkdir -p $build_dir
    mkdir -p $build_dir/include
    mkdir -p $build_dir/lib
    mkdir -p $third_party

    alibs=("$(get_brew_pkg "libsigsegv")" "$(get_brew_pkg "gmp")" "$(get_brew_pkg "libtecla")")

    for d in "${alibs[@]}"
    do
      # copy
      cp $d/include/* $build_dir/include
      cp $d/lib/* $build_dir/lib
    done

  else
    build_libsigsegv
    build_gmp
    build_buddy
    build_tecla
  fi

  # build_libpoly
  # build_libncurses

  # build smt solver
  # build_z3
  # build_yices2

  rm -rf "$build_dir"/lib/*.so*
  rm -rf "$build_dir"/lib/*.dylib*
}

build_deps() {

  build_libsigsegv
  build_gmp
  build_buddy
  build_tecla

  # build_libpoly
  # build_libncurses

  # build smt solver
  # build_z3
  # build_yices2

  rm -rf "$build_dir"/lib/*.so*
  rm -rf "$build_dir"/lib/*.dylib*
}

patch_src() {
  progress "Apply patchings"
  maude_src_dir="$top_dir/maude-bindings"
  patch_src_dir="$top_dir/src/patch"

  cd "$maude_src_dir/"
  (
    patch -p0 < $top_dir/src/patch/b-*.patch
  )

  cd "$maude_src_dir/subprojects/maudesmc/"
  (
    patch -p0 < $top_dir/src/patch/c-*.patch
    patch -p0 < $top_dir/src/patch/d-*.patch
  )
}

mk_patch() {
  progress "Make patch for Maude as a library"

  cd "$top_dir/maude-bindings/"
  (
    git diff --no-prefix ":^subprojects" ":^pyproject.toml" ":^README.md" > $top_dir/src/patch/b-$(git log -1 --pretty=format:"%h").patch
  )

  cd "$top_dir/maude-bindings/subprojects/maudesmc/"
  (
    git diff --no-prefix ":^src" > $top_dir/src/patch/c-$(git log -1 --pretty=format:"%h").patch
    git diff --no-prefix src/ > $top_dir/src/patch/d-$(git log -1 --pretty=format:"%h").patch
  )
}

prepare() {
  pip install scikit-build ninja cmake meson===1.4.1 swig build
  git clone https://github.com/fadoss/maude-bindings.git

  # currently use specific version
  cd maude-bindings
  git submodule update --init
}

build_maude_lib() {
  maude_src_dir="$top_dir/maude-bindings/subprojects/maudesmc/src"
  cp -r "$src_dir/Extension" "$smc_dir/src"
  # py_lib="$(python3-config --prefix)/lib"
  py_inc="$(python -c "from sysconfig import get_paths; print(get_paths()['include'])")"

  cd maude-bindings/subprojects/maudesmc
  (
    rm -rf release

    os=$(uname)
    if [[ "$os" == "Darwin" ]]; then
      arch -arm64 meson setup release --buildtype=custom -Dcpp_args="-fno-stack-protector -fstrict-aliasing" \
            -Dextra-lib-dirs="$build_dir/lib" \
            -Dextra-include-dirs="$build_dir/include, $py_inc" \
            -Dstatic-libs='buddy, gmp, sigsegv' \
            -Dwith-smt='pysmt' \
            -Dwith-ltsmin=disabled \
            -Dwith-simaude=disabled \
            -Dc_args='-mno-thumb' \
            -Dc_link_args="-Wl,--export-dynamic" \
            -Dcpp_link_args="-mmacosx-version-min=15.0 -Wl,-x -u ___gmpz_get_d -L"$build_dir"/lib -lgmp"
    else
      meson setup release --buildtype=custom -Dcpp_args="-fno-stack-protector -fstrict-aliasing" \
            -Dextra-lib-dirs="$build_dir/lib" \
            -Dextra-include-dirs="$build_dir/include, $py_inc" \
            -Dstatic-libs='buddy, gmp, sigsegv' \
            -Dwith-smt='pysmt' \
            -Dwith-ltsmin=disabled \
            -Dwith-simaude=disabled \
            -Dc_args='-mno-thumb' \
            -Dc_link_args="-Wl,--export-dynamic" \
            -Dcpp_link_args="-Wl,-x -u __gmpz_get_d -L"$build_dir"/lib -lgmp"
    fi
    cd release && ninja
  )
}

# build maude
# TODO: need to update in ubuntu
build_maude() {
  progress "Copying source files to Maude directory"
  cp -r "$src_dir/Extension" "$maude_raw_dir"

  # cd "$top_dir/test-build/Maude"
  # aclocal
  # autoconf
  # autoheader
  # automake --add-missing
  # automake

  mkdir -p "$top_dir/test-build/Maude/Out"

  # need to build first with dynamic options,
  # and then do the static building.
  # maude needs dynamic library for TCP.
  # Bstatic will compile only z3 with libz3.a not libz3.so
  #
  # CXXFLAGS if you need dynamic library linking,
  # you need to add libz3.so to lib dir and remove -pthread option
  #
  # ./configure CXXFLAGS="-std=c++11 -I$include_dir -pthread" \
  #	    LDFLAGS="-L$lib_dir" \
  #	    "$@"
  cd "$top_dir/test-build/Maude/Out"
  (
      ../configure --with-yices2=no --with-cvc4=no --with-z3=yes --enable-compiler \
      BISON="/opt/homebrew/opt/bison/bin/bison" \
      CPPFLAGS="-I$include_dir" CXXFLAGS="-g -fno-stack-protector -fstrict-aliasing -mmacosx-version-min=15.0 -std=c++11" \
      LDFLAGS="-L$lib_dir -Wl,-no_pie" \
      Z3_LIB="$lib_dir/libz3.a" \
      GMP_LIBS="$lib_dir/libgmpxx.a $lib_dir/libgmp.a" \
    "$@"

    # -Wall -O3

    make
    make check
    cd src/Main
    cp maude maude.darwin64
  )
}


build_maude_se() {
  maudesmc_dir="$top_dir/maude-bindings/subprojects/maudesmc"
  swig_src_dir="$top_dir/maude-bindings/swig"

  mkdir -p $maudesmc_dir/build
  mkdir -p $maudesmc_dir/installdir/lib

  cp $maudesmc_dir/release/config.h $maudesmc_dir/build
  cp $maudesmc_dir/release/libmaude.so $maudesmc_dir/installdir/lib
  cp $maudesmc_dir/release/libmaude.dylib $maudesmc_dir/installdir/lib

  cp "$top_dir/src/pyproject.toml" $top_dir/maude-bindings
  cp "$top_dir/README.md" $top_dir/maude-bindings

  cp "$src_dir/swig/rwsmt.i" "$swig_src_dir"
  cp "$src_dir/swig/core.i"   "$swig_src_dir"
  cp "$src_dir/Extension/pysmt.hh" "$top_dir/maude-bindings/src" 

  cd maude-bindings 
  (
    rm -rf dist/ maude.egg-info/ _skbuild/
    CMAKE_ARGS="-DBUILD_LIBMAUDE=OFF -DEXTRA_INCLUDE_DIRS=$build_dir/include -DMAUDE_SE_INSTALL_FILES=$top_dir/src" python -m build
    # python setup.py bdist_wheel -DBUILD_LIBMAUDE=OFF \
          # -DEXTRA_INCLUDE_DIRS="$build_dir/include"
  )

  cd ..
  # rm -rf ./out
  mkdir -p ./out
  cp -r ./maude-bindings/dist/* ./out
}

# build gmp
build_gmp_raw() {
  progress "Building gmp library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading gmp 6.3.0"
    get_gnu "gmp" "6.3.0" "tar.xz"
    cd "$third_party/gmp-6.3.0"

    if [ "$OS" == "Darwin" ]; then
      ./configure CFLAGS="-g -fno-stack-protector -O3 -mmacosx-version-min=10.15" \
                  CXXFLAGS="-g -fno-stack-protector -O3 -mmacosx-version-min=10.15 -std=c++11" \
                  --prefix="$build_dir" \
                  --enable-cxx \
                  --enable-fat \
                  --enable-shared=yes
    else    
      ./configure --prefix="$build_dir" \
                  --enable-cxx --enable-fat \
                  --disable-shared --enable-static \
                  --build=x86_64-pc-linux-gnu
    fi

    make
    make check
    make install
  )
}

# build gmp
build_gmp() {
  progress "Building gmp library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading gmp 6.1.2"
    get_gnu "gmp" "6.1.2" "tar.xz"
    cd "$third_party/gmp-6.1.2"

    ./configure --prefix="$build_dir" CFLAGS=-fPIC CXXFLAGS=-fPIC \
		--enable-cxx --enable-fat --disable-shared --enable-static --build=x86_64-pc-linux-gnu
          
    make -j4
    make check
    make install
  )
}

build_z3() {
  progress "Building z3"
  mkdir -p $build_dir
  mkdir -p "$third_party"
  ( progress "Downloading Z3"
    git clone https://github.com/Z3Prover/z3 "$third_party/z3"
    
    cd "$third_party/z3"
    python scripts/mk_make.py --prefix="$build_dir" --staticlib CPPFLAGS="-O3 -fno-stack-protector" CXXFLAGS="-O3 -fno-stack-protector"
    cd build
    make -j4
    make install
  )

  rm -rf $build_dir/lib/libz3*.so*
  rm -rf $build_dir/lib/libz3*.dylib
}

build_yices2() {
  progress "Building Yices"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading Yices2"
    git clone https://github.com/SRI-CSL/yices2 "$third_party/yices2"
    
    cd "$third_party/yices2"
    autoconf
    ./configure --prefix=$build_dir --with-static-gmp=$lib_dir/libgmp.a --with-static-gmp-include-dir=$include_dir \
                CFLAGS="-g -fno-stack-protector -O3" LDFLAGS="-L$lib_dir" CPPFLAGS="-I$include_dir"

    make -j4
    make install
  )

  rm -rf $build_dir/lib/libyices*.so*
  rm -rf $build_dir/lib/libyices*.dylib
}

#
build_buddy_raw() {
  progress "Building BuDDy library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading BuDDy 2.4"
    buddy_dir="$third_party/buddy-2.4"

    curl -L https://github.com/utwente-fmt/buddy/releases/download/v2.4/buddy-2.4.tar.gz > "$buddy_dir.tar.gz"
    tar -xvzf "$buddy_dir.tar.gz" -C "$third_party"
    rm -rf "$buddy_dir.tar.gz"

    cd "$buddy_dir"
  
    if [ "$OS" == "Darwin" ]; then
      ./configure LDFLAGS=-lm \
                  CFLAGS="-g -fno-stack-protector -O3 -mmacosx-version-min=10.15" \
                  CXXFLAGS="-g -fno-stack-protector -O3 -mmacosx-version-min=10.15 -std=c++11" \
                  --prefix=$build_dir \
                  --disable-shared
      make
      make check
      chmod a+x ./tools/install-sh
      make install
    else
      ./configure LDFLAGS=-lm \
                  CFLAGS="-g -fno-stack-protector -O3" \
                  CXXFLAGS="-g -fno-stack-protector -O3" \
                  --prefix="$build_dir" \
                  --disable-shared
      make
      make check
      make install
    fi
  )
}

build_buddy() {
  progress "Building BuDDy library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading BuDDy 2.4"
    buddy_dir="$third_party/buddy-2.4"

    curl -L https://github.com/utwente-fmt/buddy/releases/download/v2.4/buddy-2.4.tar.gz > "$buddy_dir.tar.gz"
    tar -xvzf "$buddy_dir.tar.gz" -C "$third_party"
    rm -rf "$buddy_dir.tar.gz"

    cd "$buddy_dir"

    # replace old config guess to latest one
    rm -rf config.guess config.sub

    wget -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
    wget -O config.sub 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'

    ./configure CFLAGS="-fPIC" CXXFLAGS="-fPIC" --includedir="$include_dir" --libdir="$lib_dir" --disable-shared
    
    make -j4
    make check
    chmod a+x ./tools/install-sh
    make install
  )
}

# build tecla
build_tecla_raw() {
  progress "Building tecla library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading Tecla 1.6.3"
    tecla_dir="$third_party/libtecla"
  
    curl -o "$tecla_dir.tar.gz" https://sites.astro.caltech.edu/~mcs/tecla/libtecla-1.6.3.tar.gz
    tar -xvzf "$tecla_dir.tar.gz" -C "$third_party"
    rm -rf "$tecla_dir.tar.gz" 
    
    cd "$tecla_dir"

    if [ "$OS" == "Darwin" ]; then
      # Add #include <sys/ioctl.h> as the first line of enhance.c
      echo -e "#include <sys/ioctl.h>\n$(cat enhance.c)" > enhance.c

      # replace old config guess to latest one
      rm -rf config.guess config.sub

      wget -O config.guess 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.guess;hb=HEAD'
      wget -O config.sub 'https://git.savannah.gnu.org/gitweb/?p=config.git;a=blob_plain;f=config.sub;hb=HEAD'

      ./configure CFLAGS="-fno-stack-protector -O3 -mmacosx-version-min=10.15" \
                  --prefix=$build_dir
      make
      make install
    else
      ./configure CFLAGS="-g -fno-stack-protector -O3" \
                  --prefix=$build_dir
      make
      make install
    fi
  )
}

# build tecla
build_tecla() {
  progress "Building tecla library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading Tecla 1.6.3"
    tecla_dir="$third_party/libtecla"
  
    curl -o "$tecla_dir.tar.gz" https://sites.astro.caltech.edu/~mcs/tecla/libtecla-1.6.3.tar.gz
    tar -xvzf "$tecla_dir.tar.gz" -C "$third_party"
    rm -rf "$tecla_dir.tar.gz" 
    
    cd "$tecla_dir"

    ./configure CXXFLAGS-"-fPIC" CFLAGS="-fPIC -g -fno-stack-protector -O3" \
                --prefix=$build_dir
    make
    make install
  )
}

# build libsigsegv
build_libsigsegv_raw() {
  progress "Building libsigsegv library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading Libsigsegv 2.12"
    get_gnu "libsigsegv" "2.12" "tar.gz"
    sigsegv_dir="$third_party/libsigsegv-2.12"
    
    cd "$sigsegv_dir"

    if [ "$OS" == "Darwin" ]; then
      ./configure CFLAGS="-g -fno-stack-protector -O3 -mmacosx-version-min=10.15" \
                  --prefix="$build_dir" --enable-shared=no
    else
      ./configure CFLAGS="-g -fno-stack-protector -O3" \
                  --prefix="$build_dir" --enable-shared=no
    fi

    make -j4
    make check
    make install
  )
}

# build libsigsegv
build_libsigsegv() {
  progress "Building libsigsegv library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading Libsigsegv 2.12"
    get_gnu "libsigsegv" "2.12" "tar.gz"
    sigsegv_dir="$third_party/libsigsegv-2.12"
    
    cd "$sigsegv_dir"

    ./configure CXXFLAGS="-fPIC" CFLAGS="-fPIC -g -fno-stack-protector -O3" \
                --prefix="$build_dir" --enable-shared=no
    
    make -j4
    make check
    make install
  )
}

# build libncurses
build_libncurses() {
  progress "Building libncurses library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading Ncurses 6.1"
    get_gnu "ncurses" "6.1" "tar.gz"
    libncurses_dir="$third_party/ncurses-6.1"
    
    cd "$libncurses_dir"
    ./configure --prefix="$build_dir" --datadir="/usr/share" \
    --with-terminfo-dirs="/usr/lib/share:/lib/terminfo:/etc/terminfo" \
    --with-default-terminfo-dir="/usr/share/terminfo" --with-normal --disable-db-install
    
    make -j4
    make install
  )
}

# build libpoly for yices non linear support
build_libpoly() {
  progress "Building libpoly library"
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading Libpoly"
    git clone https://github.com/SRI-CSL/libpoly.git "$third_party/libpoly"
    
    cd "$third_party/libpoly/build"
    cmake .. -DCMAKE_BUILD_TYPE="Release" -DCMAKE_INSTALL_PREFIX="$build_dir" -DGMP_LIBRARY="$lib_dir/libgmp.a" -DGMP_INCLUDE_DIR="$include_dir" -DCMAKE_C_FLAGS="-fPIC" -DCMAKE_CXX_FLAGS="-fPIC"
    make
    make install
  )
}

build_cudd() {
  progress "Building Cudd"
  cudd_dir=$third_party/cudd-cudd-3.0.0
  mkdir -p "$build_dir"
  mkdir -p "$third_party"
  ( progress "Downloading Cudd 3.0.0"
    cudd_name="cudd-3.0.0"
  
    curl -L https://github.com/ivmai/cudd/archive/refs/tags/$cudd_name.tar.gz > "$third_party/$cudd_name.tar.gz"
    tar -xvzf "$third_party/$cudd_name.tar.gz" -C "$third_party"
    rm -rf "$third_party/$cudd_name.tar.gz"
    
    cd "$third_party/cudd-$cudd_name"
    ./configure CFLAGS="-fPIC" CXXFLAGS="-fPIC" --prefix="$build_dir"

    make -j4 check
    make install
  )
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

# Follow the below steps
#  1. prepare
#  2. build_deps
#  3. patch_src
#  4. build_maude_lib
#  5. build_maude_se

# Main
# ----

build_command="$1" ; shift
case "$build_command" in
    prep)               prepare                   "$@" ;;
    deps)               build_deps                "$@" ;;
    deps-wf)            build_deps_workflows      "$@" ;;
    patch)              patch_src                 "$@" ;;
    build-maude)        build_maude_lib           "$@" ;;
    build-maude-se)     build_maude_se            "$@" ;;
    mk-patch)           mk_patch                  "$@" ;;
    build-test)         build_maude               "$@" ;;
    *)      echo "
    usage: $0 [prep|deps|patch|build]
           $0 script <options>

    $0 prep     :   prepare Maude-as-a-library
    $0 deps     :   prepare Maude dependencies
    $0 patch    :   patch Maude-as-a-library
    $0 compile  :   compile Maude-SE
    $0 maude-se :   build Maude-SE
" ;;
esac