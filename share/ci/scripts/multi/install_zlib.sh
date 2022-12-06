#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

ZLIB_VERSION="$1"
INSTALL_TARGET="$2"

ZLIB_MAJOR_MINOR=$(echo "${ZLIB_VERSION}" | cut   -d. -f-2)
ZLIB_MAJOR=$(echo "${ZLIB_VERSION}" | cut   -d. -f-1)
ZLIB_MINOR=$(echo "${ZLIB_MAJOR_MINOR}" | cut   -d. -f2-)
ZLIB_PATCH=$(echo "${ZLIB_VERSION}" | cut   -d. -f3-)
ZLIB_VERSION_U="${ZLIB_MAJOR}.${ZLIB_MINOR}.${ZLIB_PATCH}"

git clone https://github.com/madler/zlib
cd zlib

if [ "$ZLIB_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/v${ZLIB_VERSION_U} -b v${ZLIB_VERSION_U}
fi

mkdir build
cd build

cmake -DCMAKE_BUILD_TYPE=Release \
      ${INSTALL_TARGET:+"-DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}""} \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      -DBUILD_SHARED_LIBS=OFF \
      ../.
cmake --build . \
      --target install \
      --config Release \
      --parallel 2

cd ../..
rm -rf zlib
