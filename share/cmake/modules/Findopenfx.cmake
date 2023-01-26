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
# The dynamic libraries will be located by default.
#
# If the library is not installed in a standard path, you can do the following the help
# the find module:
#
# If the package provides a configuration file, use -Dopenfx_DIR=<path to folder>.
# If it doesn't provide it, try -Dopenfx_ROOT=<path to folder with lib and includes>.
# Alternatively, try -Dopenfx_LIBRARY=<path to lib file> and -Dopenfx_INCLUDE_DIR=<path to folder>.
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
