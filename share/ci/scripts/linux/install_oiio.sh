#!/usr/bin/env bash

set -ex

OIIO_VERSION="$1"

git clone https://github.com/OpenImageIO/oiio.git
cd oiio

if [ "$OIIO_VERSION" != "latest" ]; then
    git checkout tags/Release-${OIIO_VERSION} -b Release-${OIIO_VERSION}
fi

mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_CXX_FLAGS="-Wno-error" \
      -DOIIO_BUILD_TOOLS=OFF \
      -DOIIO_BUILD_TESTS=OFF \
      -DVERBOSE=ON \
      -DBoost_NO_BOOST_CMAKE=ON \
      ../.
make -j4
sudo make install

cd ../..
rm -rf oiio
