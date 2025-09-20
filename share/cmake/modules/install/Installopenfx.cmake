# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Install openfx
#
# Variables defined by this module:
#   openfx_FOUND          - Indicate whether the library was found or not
#   openfx_INCLUDE_DIR    - Location of the header files
#   openfx_VERSION        - Library's version
#
# Global targets defined by this module:
#   openfx::module         
#


###############################################################################
### Create target

if(NOT TARGET openfx::module)
    add_library(openfx::module INTERFACE IMPORTED GLOBAL)
    set(_openfx_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT openfx_FOUND AND OCIO_INSTALL_EXT_PACKAGES AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${PROJECT_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${PROJECT_BINARY_DIR}/ext/build")
    set(_openfx_INSTALL_DIR "${_EXT_BUILD_ROOT}/openfx/src/openfx_install")

    # Set find_package standard args
    set(openfx_FOUND TRUE)
    if(OCIO_openfx_RECOMMENDED_VERSION)
        set(openfx_VERSION ${OCIO_openfx_RECOMMENDED_VERSION})
    else()
        set(openfx_VERSION ${openfx_FIND_VERSION})
    endif()
    set(openfx_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}/openfx")

    if(_openfx_TARGET_CREATE)
        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${openfx_INCLUDE_DIR})

        include(VersionUtils)
        split_version_string(${openfx_VERSION} openfx)

        ExternalProject_Add(openfx_install
            GIT_REPOSITORY "https://github.com/ofxa/openfx.git"
            # The latest version from 2015 is OFX_Release_1_4_TAG.
            # The latest version from 2024 is OFX_Release_1.5.
            # Need to be careful since older version might have the patch number in the tag.
            # There don't seem to be enough consistency in tag names that we can rely on.
            GIT_TAG "OFX_Release_${openfx_VERSION_MAJOR}.${openfx_VERSION_MINOR}"
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
        if(OCIO_VERBOSE)
            message(STATUS "Installing openfx: ${openfx_INCLUDE_DIR} (version \"${openfx_VERSION}\")")
        endif()
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
