# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install OpenEXR.
#
# Variables defined by this module:
#   OpenEXR_FOUND - If FALSE, do not try to link to OpenEXR
#   OpenEXR_LIBRARY - OpenEXR library to link to
#   OpenEXR_INCLUDE_DIR - Where to find OpenEXR headers
#   OpenEXR_VERSION - The version of the library
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
# By default, the dynamic libraries of OpenEXR will be found. To find the
# static ones instead, you must set the OpenEXR_STATIC_LIBRARY variable to
# TRUE before calling find_package(OpenEXR ...).
#
# If OpenEXR is not installed in a standard path, you can use the
# OpenEXR_ROOT variable to tell CMake where to find it. If it is not found
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, OpenEXR will be
# downloaded, statically-built, and used to generate
# libOpenColorIOimageioapphelpers.
#

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_OpenEXR_REQUIRED_VARS OpenEXR_LIBRARY)
    set(_OpenEXR_LIB_VER "${OpenEXR_FIND_VERSION_MAJOR}_${OpenEXR_FIND_VERSION_MINOR}")

    if(NOT DEFINED OpenEXR_ROOT)
        # Search for OpenEXRConfig.cmake
        find_package(OpenEXR ${OpenEXR_FIND_VERSION} CONFIG QUIET)
    endif()

    if(OpenEXR_FOUND)
        get_target_property(OpenEXR_LIBRARY OpenEXR::OpenEXR LOCATION)
        get_target_property(OpenEXR_INCLUDE_DIR OpenEXR::OpenEXR INTERFACE_INCLUDE_DIRECTORIES)
        
        # IMPORTED_GLOBAL property must be set to TRUE since alisasing a non-global imported target
        # is not possible until CMake 3.18+.
        set_target_properties(OpenEXR::OpenEXR PROPERTIES IMPORTED_GLOBAL TRUE)
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(OpenEXR_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(OpenEXR
        REQUIRED_VARS
            ${_OpenEXR_REQUIRED_VARS}
        VERSION_VAR
            OpenEXR_VERSION
    )
endif()
