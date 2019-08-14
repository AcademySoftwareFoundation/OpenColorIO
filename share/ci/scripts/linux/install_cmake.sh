#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set -ex

CMAKE_VERSION="$1"

curl --location "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.sh" -o "cmake.sh" &&\
sudo sh cmake.sh --skip-license --prefix=/usr/local
rm cmake.sh
