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

if [[ $OSTYPE == 'darwin'* ]]; then
    cmake -DCMAKE_BUILD_TYPE=Release \
        ${INSTALL_TARGET:+"-DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}""} \
        -DCMAKE_CXX_STANDARD=14 \
        -DCMAKE_C_COMPILER=$(brew --prefix llvm@15)/bin/clang \
        -DCMAKE_CXX_COMPILER=$(brew --prefix llvm@15)/bin/clang++ \
        -DOSL_BUILD_TESTS=OFF \
        -DVERBOSE=ON \
        -DSTOP_ON_WARNING=OFF \
        -DBoost_NO_BOOST_CMAKE=ON \
        -DLLVM_ROOT=$(brew --prefix llvm@15) \
        ../.
else # not macOS
    cmake -DCMAKE_BUILD_TYPE=Release \
        ${INSTALL_TARGET:+"-DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}""} \
        -DCMAKE_CXX_STANDARD=14 \
        -DOSL_BUILD_TESTS=OFF \
        -DVERBOSE=ON \
        -DSTOP_ON_WARNING=OFF \
        -DBoost_NO_BOOST_CMAKE=ON \
        ../.
fi

# OSL 1.13+ yield a permission error on mac OS.
# "file cannot create directory: /usr/local/cmake.  Maybe need administrative privileges."
if [[ $OSTYPE == 'darwin'* ]]; then
sudo cmake --build . \
           --target install \
           --config Release \
           --parallel 2
else # not macOS
cmake --build . \
      --target install \
      --config Release \
      --parallel 2
fi

cd ../..
rm -rf OpenShadingLanguage
