#!/usr/bin/env bash

# failed on any error 
set -euo pipefail

top_dir="$(pwd)"
work_dir="$top_dir/.build-wheel"
bindings_dir="$work_dir/sources/maude-bindings"
build_dir="$work_dir/install"
package_src_dir="$build_dir/package-src"

refversion=cp311-cp311
versions=(cp310-cp310 cp311-cp311 cp312-cp312 cp313-cp313 cp314-cp314)
for version in "${versions[@]}"; do
    if [[ ! -x "/opt/python/${version}/bin/python" ]]; then
        echo "error: required Python is unavailable: $version" >&2
        exit 1
    fi
done

yum install flex bison -y

export PATH="/opt/python/${refversion}/bin:$PATH"

python -m pip install -r "$top_dir/build/requirements.txt"
python -m pip install --upgrade wheel auditwheel

command -v meson
command -v ninja

./build/build.sh prep
./build/build.sh deps
./build/build.sh build-maude
./build/build.sh prep-build-maude-se

cd "$bindings_dir"
(
    for version in "${versions[@]}"; do
        /opt/python/${version}/bin/python -m pip install --upgrade scikit-build-core scikit-build ninja cmake meson swig build
        CMAKE_ARGS="-DBUILD_LIBMAUDE=OFF -DEXTRA_INCLUDE_DIRS=$build_dir/include -DMAUDE_SE_INSTALL_FILES=$package_src_dir" \
            /opt/python/${version}/bin/python -m pip wheel -w dist --no-deps .
    done

    for whl in dist/*linux_*.whl; do
        /opt/python/${refversion}/bin/auditwheel repair $whl -w .dist/
    done
)

cd "$top_dir"

mkdir -p ./out
cp -r "$bindings_dir"/.dist/* ./out
