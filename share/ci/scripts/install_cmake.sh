#!/usr/bin/env bash

set -ex

CMAKE_MAJOR="$1"
CMAKE_MINOR="$2"
CMAKE_PATCH="$3"
CMAKE_MAJOR_MINOR="${CMAKE_MAJOR}.${CMAKE_MINOR}"
CMAKE_VERSION="${CMAKE_MAJOR}.${CMAKE_MINOR}.${CMAKE_PATCH}"

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
