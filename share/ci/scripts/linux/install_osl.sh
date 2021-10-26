#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

OSL_VERSION="$1"

git clone https://github.com/AcademySoftwareFoundation/OpenShadingLanguage.git
cd OpenShadingLanguage

if [ "$OSL_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/Release-${OSL_VERSION} -b Release-${OSL_VERSION}
fi

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release \
      -DOSL_BUILD_TESTS=OFF \
      -DVERBOSE=ON \
      -DSTOP_ON_WARNING=OFF \
      -DBoost_NO_BOOST_CMAKE=ON \
      ../.
make -j4
sudo make install

cd ../..
rm -rf OpenShadingLanguage
