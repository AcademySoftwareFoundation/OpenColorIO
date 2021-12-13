# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install openfx
#
# Variables defined by this module:
#   openfx_FOUND - If FALSE, do not try to include openfx
#   openfx_INCLUDE_DIR - Where to find ofxCore.h
#   openfx_VERSION - The version of the library
#
# Targets defined by this module:
#   openfx::module - IMPORTED target, if found
#
# If openfx is not installed in a standard path, you can use the 
# openfx_ROOT variable to tell CMake where to find it. If it is not found 
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, openfx will be 
# downloaded at build time.
#

if(NOT TARGET openfx::module)
    add_library(openfx::module INTERFACE IMPORTED GLOBAL)
    set(_openfx_TARGET_CREATE TRUE)
endif()

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Find include directory
    find_path(openfx_INCLUDE_DIR
        NAMES
            ofxCore.h
        HINTS
            ${openfx_ROOT}
        PATH_SUFFIXES
            include
            openfx/include
            openfx
            include/openfx
    )

    # TODO: Find method for setting openfx_VERSION from found openfx headers.
    #       There are some changelog hints with version references, but 
    #       nothing appears to concretely indicate the current version.

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(openfx_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(openfx
        REQUIRED_VARS 
            openfx_INCLUDE_DIR
    )
endif()

###############################################################################
### Install package from source ###

if(NOT openfx_FOUND AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")
    set(_openfx_INSTALL_DIR "${_EXT_BUILD_ROOT}/openfx/src/openfx_install")

    # Set find_package standard args
    set(openfx_FOUND TRUE)
    set(openfx_VERSION ${openfx_FIND_VERSION})
    set(openfx_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}/openfx")

    if(_openfx_TARGET_CREATE)
        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${openfx_INCLUDE_DIR})

        ExternalProject_Add(openfx_install
            GIT_REPOSITORY "https://github.com/ofxa/openfx.git"
            GIT_TAG "OFX_Release_${openfx_FIND_VERSION_MAJOR}_${openfx_FIND_VERSION_MINOR}_TAG"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/openfx"
            BUILD_BYPRODUCTS ${openfx_INCLUDE_DIR}
            CONFIGURE_COMMAND ""
            BUILD_COMMAND
                ${CMAKE_COMMAND} -E copy_directory
                "${_EXT_BUILD_ROOT}/openfx/src/openfx_install/include"
                "${openfx_INCLUDE_DIR}"
            INSTALL_COMMAND ""
            CMAKE_ARGS ${openfx_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(openfx::module openfx_install)
        message(STATUS "Installing openfx: ${openfx_INCLUDE_DIR} (version \"${openfx_VERSION}\")")
    endif()
endif()

###############################################################################
### Configure target ###

if(_openfx_TARGET_CREATE)
    set_target_properties(openfx::module PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${openfx_INCLUDE_DIR}
    )

    mark_as_advanced(openfx_INCLUDE_DIR)
endif()
