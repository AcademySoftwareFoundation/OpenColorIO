#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

PYSTRING_VERSION="$1"

git clone https://github.com/imageworks/pystring.git
cd pystring

if [ "$PYSTRING_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/v${PYSTRING_VERSION} -b v${PYSTRING_VERSION}
fi

cp ../share/cmake/projects/Buildpystring.cmake CMakeLists.txt

mkdir build
cd build
cmake -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_CXX_FLAGS="-fPIC" \
      ../.
make -j4
sudo make install

cd ../..
rm -rf pystring
