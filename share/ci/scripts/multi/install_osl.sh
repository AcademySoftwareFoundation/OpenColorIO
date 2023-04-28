#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

OSL_VERSION="$1"
INSTALL_TARGET="$2"

git clone https://github.com/AcademySoftwareFoundation/OpenShadingLanguage.git
cd OpenShadingLanguage

if [ "$OSL_VERSION" == "latest" ]; then
    git checkout release
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/Release-${OSL_VERSION} -b Release-${OSL_VERSION}
fi

mkdir build
cd build
# FIXME: Revert OSL_BUILD_TESTS to OFF when OSL 1.12 is released
# CMake configure fails when tests are off, only fixed in 1.12 dev branch

if [[ $OSTYPE == 'darwin'* ]]; then
    cmake -DCMAKE_BUILD_TYPE=Release \
        ${INSTALL_TARGET:+"-DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}""} \
        -DCMAKE_CXX_STANDARD=14 \
        -DCMAKE_C_COMPILER=$(brew --prefix llvm@15)/bin/clang \
        -DCMAKE_CXX_COMPILER=$(brew --prefix llvm@15)/bin/clang++ \
        -DOSL_BUILD_TESTS=ON \
        -DVERBOSE=ON \
        -DSTOP_ON_WARNING=OFF \
        -DBoost_NO_BOOST_CMAKE=ON \
        -DLLVM_ROOT=$(brew --prefix llvm@15) \
        ../.
else # not macOS
    cmake -DCMAKE_BUILD_TYPE=Release \
        ${INSTALL_TARGET:+"-DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}""} \
        -DCMAKE_CXX_STANDARD=14 \
        -DOSL_BUILD_TESTS=ON \
        -DVERBOSE=ON \
        -DSTOP_ON_WARNING=OFF \
        -DBoost_NO_BOOST_CMAKE=ON \
        ../.
fi

cmake --build . \
      --target install \
      --config Release \
      --parallel 2

cd ../..
rm -rf OpenShadingLanguage
