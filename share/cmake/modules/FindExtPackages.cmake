# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# All find modules used in this module support typical find_package
# behavior or installation of packages from external projects, as configured 
# by the OCIO_INSTALL_EXT_PACKAGES option.
#

include(Colors)
include(ocio_handle_dependency)

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

if (APPLE)
    # Store the previous value of CMAKE_FIND_FRAMEWORK and CMAKE_FIND_APPBUNDLE.
    set(_PREVIOUS_CMAKE_FIND_FRAMEWORK ${CMAKE_FIND_FRAMEWORK})
    set(_PREVIOUS_CMAKE_FIND_APPBUNDLE ${CMAKE_FIND_APPBUNDLE})

    # Prioritize other paths before Frameworks and Appbundle for find_path, find_library and 
    # find_package.
    set(CMAKE_FIND_FRAMEWORK LAST)
    set(CMAKE_FIND_APPBUNDLE LAST)
endif()


message(STATUS "")
message(STATUS "Missing a dependency? Try the following possibilities:")
message(STATUS "If the package provides CMake's configuration file, use -D<pkg>_DIR=<path to folder>.")
message(STATUS "If it doesn't provide it, try -D<pkg>_ROOT=<path to folder with lib and includes>.")
message(STATUS "Alternatively, try -D<pkg>_LIBRARY=<path to lib file> and/or -D<pkg>_INCLUDE_DIR=<path to folder>.")
message(STATUS "")
message(STATUS "Please refer to the find module under share/cmake/modules for extra information.")

###############################################################################
##
## Required dependencies
##
###############################################################################
message(STATUS "")
message(STATUS "Checking for mandatory dependencies...")

# expat
# https://github.com/libexpat/libexpat
ocio_handle_dependency(  expat REQUIRED ALLOW_INSTALL
                         MIN_VERSION 2.4.1           # 2.6.0 maybe? As it's cmake 4.0 friendly
                         RECOMMENDED_VERSION 2.7.2
                         RECOMMENDED_VERSION_REASON "CVE fixes and Latest version tested with OCIO")

# yaml-cpp
# https://github.com/jbeder/yaml-cpp
ocio_handle_dependency(  yaml-cpp REQUIRED ALLOW_INSTALL
                         MIN_VERSION 0.8.0
                         RECOMMENDED_VERSION 0.8.0
                         RECOMMENDED_VERSION_REASON "Latest version tested with OCIO")

# pystring
# https://github.com/imageworks/pystring
ocio_handle_dependency(  pystring REQUIRED ALLOW_INSTALL
                         MIN_VERSION 1.1.3
                         RECOMMENDED_VERSION 1.1.4
                         RECOMMENDED_VERSION_REASON "Latest version tested with OCIO")

# Imath (>=3.1.1)
# https://github.com/AcademySoftwareFoundation/Imath
ocio_handle_dependency(  Imath REQUIRED ALLOW_INSTALL
                         MIN_VERSION 3.1.1
                         RECOMMENDED_VERSION 3.2.1
                         RECOMMENDED_VERSION_REASON "Latest version tested with OCIO")

###############################################################################
# ZLIB (https://github.com/madler/zlib)
#
# The following variables can be set:
# ZLIB_ROOT               Location of ZLIB library file and includes folder.
#                         Alternatively, ZLIB_LIBRARY and ZLIB_INCLUDE_DIR can be used.
#
# ZLIB_LIBRARY            Location of ZLIB library file.
# ZLIB_INCLUDE_DIR        Location of ZLIB includes folder.
#
# ZLIB_VERSION            ZLIB Version (CMake 3.26+)
# ZLIB_VERSION_STRING     ZLIB Version (CMake < 3.26)
#
#
# ZLIB_USE_STATIC_LIBS    Set to ON if static library is prefered (CMake 3.24+)
#
###############################################################################
ocio_handle_dependency(  ZLIB REQUIRED ALLOW_INSTALL
                         MIN_VERSION 1.2.13         # CVE fixes
                         RECOMMENDED_VERSION 1.3.1
                         RECOMMENDED_VERSION_REASON "Latest version tested with OCIO"
                         VERSION_VARS ZLIB_VERSION_STRING ZLIB_VERSION )

###############################################################################

# minizip-ng
# https://github.com/zlib-ng/minizip-ng
ocio_handle_dependency(  minizip-ng REQUIRED ALLOW_INSTALL
                         MIN_VERSION 3.0.8
                         RECOMMENDED_VERSION 4.0.10
                         RECOMMENDED_VERSION_REASON "Latest version tested with OCIO")

###############################################################################
##
## Optional dependencies
##
###############################################################################
message(STATUS "")
message(STATUS "Checking for optional dependencies...")

if(OCIO_BUILD_APPS)
    # NOTE: Depending of the compiler version lcms2 2.2 does not compile with 
    # C++17 so, if you change the lcms2 version update the code to compile 
    # lcms2 and dependencies with C++17 or higher i.e. remove the cap of C++ 
    # version in Findlcms2.cmake and src/apps/ociobakelut/CMakeLists.txt.

    # lcms2
    # https://github.com/mm2/Little-CMS
    ocio_handle_dependency(  lcms2 REQUIRED ALLOW_INSTALL
                             MIN_VERSION 2.2
                             RECOMMENDED_VERSION 2.17
                             RECOMMENDED_VERSION_REASON "Latest version tested with OCIO")
endif()

if(OCIO_BUILD_OPENFX)
    # openfx
    # https://github.com/ofxa/openfx
    ocio_handle_dependency(  openfx REQUIRED ALLOW_INSTALL
                             MIN_VERSION 1.4
                             RECOMMENDED_VERSION 1.5
                             RECOMMENDED_VERSION_REASON "Latest version tested with OCIO")
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
    ocio_handle_dependency(  Python REQUIRED
                             COMPONENTS ${_Python_COMPONENTS}
                             MIN_VERSION ${OCIO_PYTHON_VERSION}
                             RECOMMENDED_VERSION ${OCIO_PYTHON_VERSION}
                             RECOMMENDED_VERSION_REASON "Latest version tested with OCIO")

    if(OCIO_BUILD_PYTHON)
        # pybind11
        # https://github.com/pybind/pybind11
        # pybind11 2.9 fixes issues with MS Visual Studio 2022 (Debug).
        ocio_handle_dependency(  pybind11 REQUIRED ALLOW_INSTALL
                                 MIN_VERSION 2.9.2
                                 RECOMMENDED_VERSION 3.0.1)
    endif()
endif()

# Set OpenEXR Minimum version as a variable since it it used at multiple places.
if((OCIO_BUILD_APPS AND OCIO_USE_OIIO_FOR_APPS) OR OCIO_BUILD_TESTS)
    # OpenImageIO is required for OSL unit test and optional for apps.

    # OpenImageIO
    # https://github.com/OpenImageIO/oiio
    set(OIIO_VERSION "2.2.14")
    set(OIIO_RECOMMENDED_VERSION "3")

    # Supported from OIIO 2.4+. Setting this for lower versions doesn't affect anything.
    set(OPENIMAGEIO_CONFIG_DO_NOT_FIND_IMATH 1)

    set(is_OpenEXR_VERSION_valid FALSE)
    # Check for compatibility between OpenEXR and OpenImageIO.
    # Will set is_OpenEXR_VERSION_valid to TRUE if valid.
    include(CheckForOpenEXRCompatibility)

    # Do not try to find OpenImageIO if the version of OpenEXR is too old.                                    
    if (is_OpenEXR_VERSION_valid)
        ###############################################################################
        # OpenImageIO (https://github.com/OpenImageIO/oiio)
        #
        # Variables defined by OpenImageIO CMake's configuration files:
        #   OpenImageIO_FOUND          - Indicate whether the library was found or not
        #   OpenImageIO_LIB_DIR        - Library's directory
        #   OpenImageIO_INCLUDE_DIR    - Location of the header files
        #   OpenImageIO_VERSION        - Library's version
        #
        # Imported targets defined by this module, if found:
        #   OpenImageIO::OpenImageIO
        #   OpenImageIO::OpenImageIO_Util
        #
        ###############################################################################
        # Calling find_package in CONFIG mode using PREFER_CONFIG option as OIIO support
        # config file since 2.1+ and OCIO minimum version is over that.
        ocio_handle_dependency(  OpenImageIO PREFER_CONFIG
                                 MIN_VERSION ${OIIO_VERSION}
                                 RECOMMENDED_VERSION ${OIIO_RECOMMENDED_VERSION}
                                 PROMOTE_TARGET OpenImageIO::OpenImageIO)
    endif()
endif()

if(OCIO_BUILD_APPS)

    if(OCIO_USE_OIIO_FOR_APPS AND OpenImageIO_FOUND AND TARGET OpenImageIO::OpenImageIO)
        if (USE_MSVC AND OCIO_IMAGE_BACKEND STREQUAL "OpenImageIO")
            # Temporary until fixed in OpenImageIO: Mute some warnings from OpenImageIO farmhash.h
            # C4267 (level 3)	    'var' : conversion from 'size_t' to 'type', possible loss of data
            # C4244	(level 3 & 4)	'conversion' conversion from 'type1' to 'type2', possible loss of data
            target_compile_options(OpenImageIO::OpenImageIO PRIVATE /wd4267 /wd4244)
        endif()
        
        add_library(OpenColorIO::ImageIOBackend ALIAS OpenImageIO::OpenImageIO)
        set(OCIO_IMAGE_BACKEND OpenImageIO)
    else()
        ###############################################################################
        # OpenEXR (https://github.com/AcademySoftwareFoundation/openexr)
        #
        # Variables defined by OpenEXR CMake's configuration files:
        #   OpenEXR_FOUND          - Indicate whether the library was found or not
        #   OpenEXR_VERSION        - Library's version
        #
        # Imported targets defined by this module, if found:
        #   OpenEXR::Iex
        #   OpenEXR::IexConfig
        #   OpenEXR::IlmThread
        #   OpenEXR::IlmThreadConfig
        #   OpenEXR::OpenEXR
        #   OpenEXR::OpenEXRConfig
        #   OpenEXR::OpenEXRCore
        #   OpenEXR::OpenEXRUtil
        #
        ###############################################################################
        # Calling find_package in CONFIG mode using PREFER_CONFIG option.
        ocio_handle_dependency(  OpenEXR PREFER_CONFIG ALLOW_INSTALL
                                 MIN_VERSION 3.1.6
                                 RECOMMENDED_VERSION 3.3.5
                                 RECOMMENDED_VERSION_REASON "Latest version tested with OCIO"
                                 PROMOTE_TARGET OpenEXR::OpenEXR)

        if(OpenEXR_FOUND AND TARGET OpenEXR::OpenEXR)
            add_library(OpenColorIO::ImageIOBackend ALIAS OpenEXR::OpenEXR)
            set(OCIO_IMAGE_BACKEND OpenEXR)
        endif()
    endif()

    if(OCIO_IMAGE_BACKEND)
        message(STATUS "Using ${OCIO_IMAGE_BACKEND} to build ociolutimage, ocioconvert and ociodisplay.")
    endif()

endif()

# Check dependencies for OSL unit test framework (i.e. OpenImageIO and Imath) before looking
# for the Open Shading Language library.

if(OCIO_BUILD_TESTS)
    if(TARGET OpenImageIO::OpenImageIO)
        if(TARGET Imath::Imath)
            # OpenShadingLanguage
            # https://github.com/AcademySoftwareFoundation/OpenShadingLanguage
            ocio_handle_dependency(  OSL
                                     MIN_VERSION 1.13
                                     RECOMMENDED_VERSION 1.14
                                     RECOMMENDED_VERSION_REASON "Latest version tested with OCIO")
            if(NOT OSL_FOUND)
                message(WARNING "Skipping build of the OpenShadingLanguage unit tests (OSL missing)")
            endif()
        else()
            message(WARNING "Skipping build of the OpenShadingLanguage unit tests (Imath missing)")
        endif()
    else()
        message(WARNING "Skipping build of the OpenShadingLanguage unit tests (OpenImageIO missing)")
    endif()
endif()

if (APPLE)
    # Restore CMAKE_FIND_FRAMEWORK and CMAKE_FIND_APPBUNDLE values.
    set(CMAKE_FIND_FRAMEWORK ${_PREVIOUS_CMAKE_FIND_FRAMEWORK})
    set(CMAKE_FIND_APPBUNDLE ${_PREVIOUS_CMAKE_FIND_APPBUNDLE})
endif()
