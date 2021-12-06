#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

PUGIXML_VERSION="$1"
INSTALL_TARGET="$2"

git clone https://github.com/zeux/pugixml.git
cd pugixml

if [ "$PUGIXML_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/v${PUGIXML_VERSION} -b v${PUGIXML_VERSION}
fi

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      ${INSTALL_TARGET:+"-DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}""} \
      -DVERBOSE=ON \
      -DSTOP_ON_WARNING=OFF \
      -DBUILD_SHARED_LIBS=ON \
      -DBUILD_TESTS=OFF \
      ../.
cmake --build . \
      --target install \
      --config Release \
      --parallel 2

cd ../..
rm -rf pugixml
