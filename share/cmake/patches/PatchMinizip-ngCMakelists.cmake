# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# Minizip-ng cmake minimum version has to be decrease to 3.12 since the container image 
# Linux CentOS 7 VFX CY2019 image has cmake 3.12.4.
# CMake is easy to update. The image should probably be updated to a more recent version.

# cmake_minimum_required(VERSION 3.13) -> cmake_minimum_required(VERSION 3.12)
file(READ CMakeLists.txt _minizip-ng_CMAKE_DATA)
string(REPLACE "cmake_minimum_required(VERSION 3.13)" "cmake_minimum_required(VERSION 3.12)" 
    _minizip-ng_CMAKE_DATA "${_minizip-ng_CMAKE_DATA}")
file(WRITE CMakeLists.txt "${_minizip-ng_CMAKE_DATA}")