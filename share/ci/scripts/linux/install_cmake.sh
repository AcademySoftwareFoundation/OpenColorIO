#!/usr/bin/env bash

set -ex

CMAKE_VERSION="$1"

curl --location "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.sh" -o "cmake.sh" &&\
sudo sh cmake.sh --skip-license --prefix=/usr/local
rm cmake.sh
