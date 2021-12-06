#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

OPENEXR_VERSION="$1"
INSTALL_TARGET="$2"

git clone https://github.com/AcademySoftwareFoundation/openexr.git
cd openexr

if [ "$OPENEXR_VERSION" == "latest" ]; then
    git checkout release
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/v${OPENEXR_VERSION} -b v${OPENEXR_VERSION}
fi

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      ${INSTALL_TARGET:+"-DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}""} \
      -DBUILD_TESTING=OFF \
      -DOPENEXR_BUILD_UTILS=OFF \
      -DOPENEXR_VIEWERS_ENABLE=OFF \
      -DINSTALL_OPENEXR_EXAMPLES=OFF \
      -DPYILMBASE_ENABLE=OFF \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      ../.
cmake --build . \
      --target install \
      --config Release \
      --parallel 2

cd ../..
rm -rf openexr
