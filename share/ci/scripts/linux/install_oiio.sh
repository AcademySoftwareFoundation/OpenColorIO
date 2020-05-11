#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

OIIO_VERSION="$1"

# TODO: Remove this when the aswf-docker containers are upgraded to the newer
#       version of OpenJpeg. OIIO has deprecated v1 support.
sudo yum -y install openjpeg2-devel

git clone https://github.com/OpenImageIO/oiio.git
cd oiio

if [ "$OIIO_VERSION" == "latest" ]; then
    LATEST_TAG=$(git describe --abbrev=0 --tags)
    git checkout tags/${LATEST_TAG} -b ${LATEST_TAG}
else
    git checkout tags/Release-${OIIO_VERSION} -b Release-${OIIO_VERSION}
fi

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DOIIO_BUILD_TOOLS=OFF \
      -DOIIO_BUILD_TESTS=OFF \
      -DVERBOSE=ON \
      -DSTOP_ON_WARNING=OFF \
      -DBoost_NO_BOOST_CMAKE=ON \
      -DPYTHON_EXECUTABLE=$(which python) \
      ../.
make -j4
sudo make install

cd ../..
rm -rf oiio
