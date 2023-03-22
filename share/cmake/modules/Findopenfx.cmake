# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate openfx
#
# Variables defined by this module:
#   openfx_FOUND          - Indicate whether the library was found or not
#   openfx_INCLUDE_DIR    - Location of the header files
#   openfx_VERSION        - Library's version
#
# Global targets defined by this module:
#   openfx::module
#
# Usually CMake will use the dynamic library rather than static, if both are present. 
#
# If the library is not installed in a typical location where CMake will find it, you may specify 
# the location using the following method:
# -- Set -Dopenfx_ROOT to point to the directory containing the lib and include directories.
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
