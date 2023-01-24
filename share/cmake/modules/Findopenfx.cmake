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
### Create target

if(openfx_FOUND AND NOT TARGET openfx::module)
    add_library(openfx::module INTERFACE IMPORTED GLOBAL)
    set(_openfx_TARGET_CREATE TRUE)
endif()

###############################################################################
### Configure target ###

if(_openfx_TARGET_CREATE)
    set_target_properties(openfx::module PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${openfx_INCLUDE_DIR}
    )

    mark_as_advanced(openfx_INCLUDE_DIR)
endif()
