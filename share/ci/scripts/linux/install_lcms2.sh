#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

LCMS2_VERSION="$1"

git clone https://github.com/mm2/Little-CMS.git
cd Little-CMS

if [ "$LCMS2_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/lcms${LCMS2_VERSION} -b lcms${LCMS2_VERSION}
fi

cp ../share/cmake/projects/BuildLCMS2.cmake CMakeLists.txt

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DBUILD_SHARED_LIBS:BOOL=ON \
      -DCMAKE_C_FLAGS="-fPIC" \
      ../.
make -j4
sudo make install

cd ../..
rm -rf Little-CMS
