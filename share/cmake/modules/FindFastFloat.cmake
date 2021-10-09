# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install the fast_float library
#
# Variables defined by this module:
#   fast_float_FOUND - If FALSE, do not try to include fast_float.h
#   fast_float_INCLUDE_DIR - Where to find fast_float.h
#   fast_float_VERSION - The version of the library (if available)
#
# Targets defined by this module:
#   fast_float::fast_float - IMPORTED target, if found
#
# The library is include-only, there is no associated binary.
#
# If fast_float is not installed in a standard path, you can use the 
# fast_float_ROOT variable to tell CMake where to find it. If it is not found 
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, fast_float will be 
# downloaded, built, and statically-linked into libOpenColorIO at build time.
#

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_FastFloat_REQUIRED_VARS FastFloat_INCLUDE_DIR)

    if(NOT DEFINED FastFloat_ROOT)
        # Search for FastFloatConfig.cmake
        # Do notice that the CMake Config module is "FastFloat"
        # while the library is "fast_float"
        # (capital letters replaced by lowercase and underscore)
        find_package(FastFloat ${FastFloat_FIND_VERSION} CONFIG QUIET)
    endif()

    # There is no pkg-config support on fast_float.

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(FastFloat_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(FastFloat
        REQUIRED_VARS 
            ${_FastFloat_REQUIRED_VARS}
        VERSION_VAR
            FastFloat_VERSION
    )
endif()

###############################################################################
### Create target

if (NOT TARGET fast_float::fast_float)
    add_library(fast_float::fast_float INTERFACE IMPORTED GLOBAL)
    set(_FastFloat_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT FastFloat_FOUND)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(FastFloat_FOUND TRUE)
    set(FastFloat_VERSION ${FastFloat_FIND_VERSION})
    set(FastFloat_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    # This library is include-only. No need to add further flags.

    if(_FastFloat_TARGET_CREATE)
        set(FastFloat_CMAKE_ARGS
            ${FastFloat_CMAKE_ARGS}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${FastFloat_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DFASTFLOAT_TEST=OFF
            -DFASTFLOAT_SANITIZE=OFF
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(FastFloat_CMAKE_ARGS
                ${FastFloat_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            set(FastFloat_CMAKE_ARGS
                ${FastFloat_CMAKE_ARGS} -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${FastFloat_INCLUDE_DIR})

        ExternalProject_Add(FastFloat_install
            GIT_REPOSITORY "https://github.com/fastfloat/fast_float.git"
            GIT_TAG "v${FastFloat_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/fast_float"
            CMAKE_ARGS ${FastFloat_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(fast_float::fast_float FastFloat_install)
        message(STATUS "Installing fast_float: ${FastFloat_INCLUDE_DIR} (version \"${FastFloat_VERSION}\")")
    endif()
endif()

###############################################################################
### Configure target ###

if(_FastFloat_TARGET_CREATE)
    set_target_properties(fast_float::fast_float PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${FastFloat_INCLUDE_DIR}
    )

    mark_as_advanced(FastFloat_INCLUDE_DIR FastFloat_VERSION)
endif()
