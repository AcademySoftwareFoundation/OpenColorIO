#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

PYSTRING_VERSION="$1"
INSTALL_TARGET="$2"

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
cmake -DCMAKE_BUILD_TYPE=Release \
      ${INSTALL_TARGET:+"-DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}""} \
      -DBUILD_SHARED_LIBS=ON \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      ../.
cmake --build . \
      --target install \
      --config Release \
      --parallel 2

cd ../..
rm -rf pystring
