# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# All find modules used in this module support typical find_package
# behavior or installation of packages from external projects, as configured 
# by the OCIO_INSTALL_EXT_PACKAGES option.
#

###############################################################################
### Global package options ###

# Some packages register their CMake config location in the CMake User or 
# System Package Registry. We disable these search locations globally since 
# they can cause unwanted linking between multiple builds of OpenColorIO 
# when a package has previously been installed to ext/dist. Set these variables
# to OFF during cmake configuration to enable package registry use.

set(CMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY ON CACHE BOOL
    "Disable CMake User Package Registry when finding packages")

set(CMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY ON CACHE BOOL
    "Disable CMake System Package Registry when finding packages")

###############################################################################
### Packages and versions ###

# expat
# https://github.com/libexpat/libexpat
find_package(expat 2.2.8 REQUIRED)

# yaml-cpp
# https://github.com/jbeder/yaml-cpp
find_package(yaml-cpp 0.6.3 REQUIRED)

# Half (OpenEXR/IlmBase)
# https://github.com/openexr/openexr
find_package(Half 2.4.0 REQUIRED)

# pystring
# https://github.com/imageworks/pystring
find_package(pystring 1.1.3 REQUIRED)

if(OCIO_BUILD_APPS)

    # NOTE: Depending of the compiler version lcms2 2.2 does not compile with 
    # C++17 so, if you change the lcms2 version update the code to compile 
    # lcms2 and dependencies with C++17 or higher i.e. remove the cap of C++ 
    # version in Findlcms2.cmake and src/apps/ociobakelut/CMakeLists.txt.

    # lcms2
    # https://github.com/mm2/Little-CMS
    find_package(lcms2 2.2 REQUIRED)
endif()

if(OCIO_BUILD_PYTHON)

    # NOTE: Depending of the compiler version pybind11 2.4.3 does not compile 
    # with C++17 so, if you change the pybind11 version update the code to 
    # compile pybind11 and dependencies with C++17 or higher i.e. remove the 
    # cap of C++ version in FindPybind11.cmake and 
    # src/bindings/python/CMakeLists.txt.

    # pybind11
    # https://github.com/pybind/pybind11
    find_package(pybind11 2.6.1 REQUIRED)
endif()

if(OCIO_BUILD_PYTHON OR OCIO_BUILD_DOCS)

    # NOTE: We find Python once in the global scope so that it can be checked 
    # and referenced throughout the project.

    set(_Python_COMPONENTS Interpreter)
    if(OCIO_BUILD_PYTHON)
        list(APPEND _Python_COMPONENTS Development)
    endif()

    # Python
    find_package(Python REQUIRED COMPONENTS ${_Python_COMPONENTS})
endif()
