#!/usr/bin/env bash

top_dir="$(pwd)"
src_dir="$(pwd)/src"
maude_src_dir="$top_dir/maude-bindings/subprojects/maudesmc/src"

build_dir="$top_dir/.build"

yum install flex bison -y

# yum install autoconf automake autotools-dev bison build-essential flex libtool git zlib1g-dev m4 -y

./build.sh deps-wf
git clone https://github.com/fadoss/maude-bindings.git

# currently use specific version
cd maude-bindings
git submodule update --init

cd ..

./build.sh patch

refversion=cp311-cp311

export PATH="/opt/python/${refversion}/bin:$PATH"

python -m pip install --upgrade pip
python -m pip install --upgrade wheel auditwheel
python -m pip install meson==1.4.1 scikit-build ninja cmake swig build

cp -r "$src_dir/Extension" "$maude_src_dir"
py_inc="$(python -c "from sysconfig import get_paths; print(get_paths()['include'])")"

cd maude-bindings/subprojects/maudesmc
(
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
    cd release && ninja
)

cd $top_dir


versions=(cp39-cp39 cp310-cp310 cp311-cp311 cp312-cp312 cp313-cp313)

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
    for version in "${versions[@]}"; do
        /opt/python/${version}/bin/python -m pip install --upgrade scikit-build-core scikit-build ninja cmake meson===1.4.1 swig build
        CMAKE_ARGS="-DBUILD_LIBMAUDE=OFF -DEXTRA_INCLUDE_DIRS=$build_dir/include -DMAUDE_SE_INSTALL_FILES=$top_dir/src" /opt/python/${version}/bin/python -m build
    done

    for whl in dist/*linux_*.whl; do
        /opt/python/${refversion}/bin/auditwheel repair $whl -w .dist/
    done
)

cd ..
# rm -rf ./out
mkdir -p ./out
cp -r ./maude-bindings/.dist/* ./out

