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

# pystring
# https://github.com/imageworks/pystring
find_package(pystring 1.1.3 REQUIRED)

# Half
# NOTE: OCIO_USE_IMATH_HALF needs to be an integer for use in utils/Half.h.in
if(NOT OCIO_USE_OPENEXR_HALF)

    # Imath (>=3.0)
    # https://github.com/AcademySoftwareFoundation/Imath
    find_package(Imath 3.1.2 REQUIRED)
    
    set(OCIO_HALF_LIB Imath::Imath CACHE STRING "Half library target" FORCE)
    set(OCIO_USE_IMATH_HALF "1" CACHE STRING "Whether 'half' type will be sourced from the Imath library (>=v3.0)" FORCE)
else()

    # OpenEXR/IlmBase (<=2.5)
    # https://github.com/AcademySoftwareFoundation/openexr
    find_package(Half 2.4.0 REQUIRED)

    set(OCIO_HALF_LIB IlmBase::Half CACHE STRING "Half library target" FORCE)
    set(OCIO_USE_IMATH_HALF "0" CACHE STRING "Whether 'half' type will be sourced from the Imath library (>=v3.0)" FORCE)
endif()

if(OCIO_BUILD_APPS)

    # NOTE: Depending of the compiler version lcms2 2.2 does not compile with 
    # C++17 so, if you change the lcms2 version update the code to compile 
    # lcms2 and dependencies with C++17 or higher i.e. remove the cap of C++ 
    # version in Findlcms2.cmake and src/apps/ociobakelut/CMakeLists.txt.

    # lcms2
    # https://github.com/mm2/Little-CMS
    find_package(lcms2 2.2 REQUIRED)
endif()

if(OCIO_BUILD_OPENFX)
    # openfx
    # https://github.com/ofxa/openfx
    find_package(openfx 1.4 REQUIRED)
endif()

if (OCIO_PYTHON_VERSION AND NOT OCIO_BUILD_PYTHON)
    message (WARNING "OCIO_PYTHON_VERSION=${OCIO_PYTHON_VERSION} but OCIO_BUILD_PYTHON is off.")
endif ()

if(OCIO_BUILD_PYTHON OR OCIO_BUILD_DOCS)

    # NOTE: We find Python once in the global scope so that it can be checked 
    # and referenced throughout the project.

    set(_Python_COMPONENTS Interpreter)

    # Support building on manylinux docker images.
    # https://pybind11.readthedocs.io/en/stable/compiling.html#findpython-mode
    if(OCIO_BUILD_PYTHON AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.18.0")
        list(APPEND _Python_COMPONENTS Development.Module)
    elseif(OCIO_BUILD_PYTHON)
        list(APPEND _Python_COMPONENTS Development)
    endif()

    # Python
    find_package(Python ${OCIO_PYTHON_VERSION} REQUIRED
                 COMPONENTS ${_Python_COMPONENTS})

    if(OCIO_BUILD_PYTHON)
        # pybind11
        # https://github.com/pybind/pybind11
        find_package(pybind11 2.6.1 REQUIRED)
    endif()
endif()

# The presence of OpenImageIO allows additional OCIO apps and the OSL
# translation unit tests to be built.

if(OCIO_BUILD_APPS OR OCIO_BUILD_TESTS)
    # OpenImageIO
    # https://github.com/OpenImageIO/oiio

    set(OIIO_VERSION "2.1.9")

    if(OCIO_USE_OIIO_CMAKE_CONFIG)
        find_package(OpenImageIO ${OIIO_VERSION} CONFIG)
    else()
        find_package(OpenImageIO ${OIIO_VERSION})
    endif()
endif()

# Check dependencies for OSL unit test framework (i.e. OpenImageIO and Imath) before looking
# for the Open Shading Language library.

if(OCIO_BUILD_TESTS)
    if(TARGET OpenImageIO::OpenImageIO)
        if(TARGET Imath::Imath)
            # OpenShadingLanguage
            # https://github.com/AcademySoftwareFoundation/OpenShadingLanguage
            find_package(OpenShadingLanguage 1.11)
            if(NOT OSL_FOUND)
                message(WARNING "Could NOT find OpenShadingLanguage. Skipping build of the OSL unit tests.")
            endif()
        else()
            message(WARNING "Could NOT find Imath. Skipping build of the OSL unit tests.")
        endif()
    else()
        message(WARNING "Could NOT find OpenImageIO. Skipping build of the OSL unit tests.")
    endif()
endif()