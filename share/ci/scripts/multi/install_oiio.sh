#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

OIIO_VERSION="$1"
INSTALL_TARGET="$2"

git clone https://github.com/OpenImageIO/oiio.git
cd oiio

if [ "$OIIO_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/Release-${OIIO_VERSION} -b Release-${OIIO_VERSION}
fi

mkdir build
cd build
# Disable python bindings to avoid build issues on Windows
cmake -DCMAKE_BUILD_TYPE=Release \
      ${INSTALL_TARGET:+"-DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}""} \
      -DOIIO_BUILD_TOOLS=OFF \
      -DOIIO_BUILD_TESTS=OFF \
      -DVERBOSE=ON \
      -DSTOP_ON_WARNING=OFF \
      -DBoost_NO_BOOST_CMAKE=ON \
      -DUSE_PYTHON=OFF \
      ../.
cmake --build . \
      --target install \
      --config Release \
      --parallel 2

cd ../..
rm -rf oiio
