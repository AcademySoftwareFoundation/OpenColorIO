#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

OPENEXR_VERSION="$1"

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
cmake -DBUILD_TESTING=OFF \
      -DOPENEXR_BUILD_UTILS=OFF \
      -DOPENEXR_VIEWERS_ENABLE=OFF \
      -DINSTALL_OPENEXR_EXAMPLES=OFF \
      -DPYILMBASE_ENABLE=OFF \
      -DCMAKE_C_FLAGS="-fPIC" \
      -DCMAKE_CXX_FLAGS="-fPIC" \
      ../.
make -j4
sudo make install

cd ../..
rm -rf openexr
