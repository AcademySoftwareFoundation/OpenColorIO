#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

MINIZIPNG_VERSION="$1"
INSTALL_TARGET="$2"

MINIZIPNG_MAJOR_MINOR=$(echo "${MINIZIPNG_VERSION}" | cut   -d. -f-2)
MINIZIPNG_MAJOR=$(echo "${MINIZIPNG_VERSION}" | cut   -d. -f-1)
MINIZIPNG_MINOR=$(echo "${MINIZIPNG_MAJOR_MINOR}" | cut   -d. -f2-)
MINIZIPNG_PATCH=$(echo "${MINIZIPNG_VERSION}" | cut   -d. -f3-)
MINIZIPNG_VERSION_U="${MINIZIPNG_MAJOR}.${MINIZIPNG_MINOR}.${MINIZIPNG_PATCH}"

git clone https://github.com/zlib-ng/minizip-ng
cd minizip-ng

if [ "$MINIZIPNG_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/${MINIZIPNG_VERSION_U} -b ${MINIZIPNG_VERSION_U}
fi

mkdir build
cd build

cmake -DCMAKE_BUILD_TYPE=Release \
      ${INSTALL_TARGET:+"-DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}""} \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      -DBUILD_SHARED_LIBS=OFF \
      -DMZ_OPENSSL=OFF \
      -DMZ_LIBBSD=OFF \
      -DMZ_BUILD_TESTS=OFF \
      -DMZ_COMPAT=OFF \
      -DMZ_BZIP2=OFF \
      -DMZ_LZMA=OFF \
      -DMZ_LIBCOMP=OFF \
      -DMZ_ZSTD=OFF \
      -DMZ_PKCRYPT=OFF \
      -DMZ_WZAES=OFF \
      -DMZ_SIGNING=OFF \
      -DMZ_ZLIB=ON \
      -DMZ_ICONV=OFF \
      -DMZ_FETCH_LIBS=OFF \
      -DMZ_FORCE_FETCH_LIBS=OFF \
      -DZLIB_LIBRARY=${INSTALL_TARGET}/${CMAKE_INSTALL_LIBDIR} \
      -DZLIB_INCLUDE_DIR=${INSTALL_TARGET}/include \
      ../.
cmake --build . \
      --target install \
      --config Release \
      --parallel 2

cd ../..
rm -rf minizip-ng
