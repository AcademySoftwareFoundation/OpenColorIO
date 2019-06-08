#!/usr/bin/env bash

set -ex

CMAKE_VERSION="$1"
CMAKE_MAJOR_MINOR=$(echo "${CMAKE_VERSION}" | cut -d. -f-2)
CMAKE_MAJOR=$(echo "${CMAKE_VERSION}" | cut -d. -f-1)
CMAKE_MINOR=$(echo "${CMAKE_MAJOR_MINOR}" | cut -d. -f2-)
CMAKE_PATCH=$(echo "${CMAKE_VERSION}" | cut -d. -f3-)

apt remove -y cmake

mkdir _cmake
cd _cmake

wget https://cmake.org/files/v${CMAKE_MAJOR_MINOR}/cmake-${CMAKE_VERSION}.tar.gz
tar -xzvf cmake-${CMAKE_VERSION}.tar.gz

cd cmake-${CMAKE_VERSION}
./bootstrap
make -j4
make install

cd ../..
rm -rf _cmake
