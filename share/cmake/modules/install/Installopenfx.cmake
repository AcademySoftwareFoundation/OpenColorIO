# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

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

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")
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

        string(REPLACE "." ";" VERSION_LIST ${openfx_VERSION})
        list(GET VERSION_LIST 0 openfx_VERSION_MAJOR)
        list(GET VERSION_LIST 1 openfx_VERSION_MINOR)
        list(GET VERSION_LIST 2 openfx_VERSION_PATCH)

        ExternalProject_Add(openfx_install
            GIT_REPOSITORY "https://github.com/ofxa/openfx.git"
            GIT_TAG "OFX_Release_${openfx_VERSION_MAJOR}_${openfx_VERSION_MINOR}_TAG"
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
