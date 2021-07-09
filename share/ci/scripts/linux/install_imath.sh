#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

IMATH_VERSION="$1"

git clone https://github.com/AcademySoftwareFoundation/Imath.git
cd Imath

if [ "$IMATH_VERSION" == "latest" ]; then
    git checkout release
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/v${IMATH_VERSION} -b v${IMATH_VERSION}
fi

mkdir build
cd build
cmake -DBUILD_TESTING=OFF \
      -DPYTHON=OFF \
      -DCMAKE_C_FLAGS="-fPIC" \
      -DCMAKE_CXX_FLAGS="-fPIC" \
      ../.
make -j4
sudo make install

cd ../..
rm -rf Imath
