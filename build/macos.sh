#!/usr/bin/env bash

top_dir="$(pwd)"
src_dir="$(pwd)/src"
maude_src_dir="$top_dir/maude-bindings/subprojects/maudesmc/src"
maudesmc_dir="$top_dir/maude-bindings/subprojects/maudesmc"
swig_src_dir="$top_dir/maude-bindings/swig"

arch="$(uname -m)"

build_dir="$top_dir/.build"

brew install bison
brew install flex

export PATH="$(brew --prefix bison)/bin:$PATH"
export PATH="$(brew --prefix flex)/bin:$PATH"

./build.sh deps-wf
git clone https://github.com/fadoss/maude-bindings.git

# currently use specific version
cd maude-bindings
git submodule update --init

cd ..

./build.sh patch

python -m pip install --upgrade pip
python -m pip install --upgrade wheel auditwheel
python -m pip install --upgrade scikit-build-core
python -m pip install meson==1.4.1 scikit-build ninja cmake swig build

cp -r "$src_dir/Extension" "$maude_src_dir"
py_inc="$(python -c "from sysconfig import get_paths; print(get_paths()['include'])")"

cd maude-bindings/subprojects/maudesmc
(
    arch -$arch meson setup release --buildtype=custom -Dcpp_args="-fno-stack-protector -fstrict-aliasing" \
        -Dextra-lib-dirs="$build_dir/lib" \
        -Dextra-include-dirs="$build_dir/include, $py_inc" \
        -Dstatic-libs='buddy, gmp, sigsegv' \
        -Dwith-smt='pysmt' \
        -Dwith-ltsmin=disabled \
        -Dwith-simaude=disabled \
        -Dc_args='-mno-thumb' \
        -Dc_link_args="-Wl,--export-dynamic" \
        -Dcpp_link_args="-mmacosx-version-min=15.0 -Wl,-x -u ___gmpz_get_d -L"$build_dir"/lib -lgmp"
    cd release && ninja
)

cd $top_dir

mkdir -p $maudesmc_dir/build
mkdir -p $maudesmc_dir/installdir/lib

cp $maudesmc_dir/release/config.h $maudesmc_dir/build
cp $maudesmc_dir/release/libmaude.dylib $maudesmc_dir/installdir/lib

cp "$top_dir/src/pyproject.toml" $top_dir/maude-bindings
cp "$top_dir/README.md" $top_dir/maude-bindings

cp "$src_dir/swig/rwsmt.i" "$swig_src_dir"
cp "$src_dir/swig/core.i"   "$swig_src_dir"
cp "$src_dir/Extension/pysmt.hh" "$top_dir/maude-bindings/src" 

cd maude-bindings 
(
    CMAKE_ARGS="-DBUILD_LIBMAUDE=OFF -DEXTRA_INCLUDE_DIRS=$build_dir/include -DMAUDE_SE_INSTALL_FILES=$top_dir/src" ARCHFLAGS="-arch $arch" python -m build
)

cd ..

# rm -rf ./out
mkdir -p ./out
cp -r ./maude-bindings/dist/* ./out