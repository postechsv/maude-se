#!/usr/bin/env bash

top_dir="$(pwd)"
build_dir="$top_dir/.build"

yum install flex bison -y

refversion=cp311-cp311
versions=(cp38-cp38 cp39-cp39 cp310-cp310 cp311-cp311 cp312-cp312 cp313-cp313)

export PATH="/opt/python/${refversion}/bin:$PATH"

python -m pip install --upgrade pip
python -m pip install --upgrade wheel auditwheel build

./build/build.sh prep
./build/build.sh deps
./build/build.sh build-maude
./build/build.sh prep-build-maude-se

cd maude-bindings 
(
    for version in "${versions[@]}"; do
        /opt/python/${version}/bin/python -m pip install --upgrade scikit-build-core scikit-build ninja cmake meson swig build
        CMAKE_ARGS="-DBUILD_LIBMAUDE=OFF -DEXTRA_INCLUDE_DIRS=$build_dir/include -DMAUDE_SE_INSTALL_FILES=$top_dir/src" \
            /opt/python/${version}/bin/python -m pip wheel -w dist --no-deps .
    done

    for whl in dist/*linux_*.whl; do
        /opt/python/${refversion}/bin/auditwheel repair $whl -w .dist/
    done
)

cd "$top_dir"

mkdir -p ./out
cp -r ./maude-bindings/.dist/* ./out