# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate sse2neon (header-only version)
#
# Variables defined by this module:
#   sse2neon_FOUND          - Indicate whether the library was found or not
#   sse2neon_INCLUDE_DIR    - Location of the header files
#
# Global targets defined by this module:
#   sse2neon
###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Find include directory
    find_path(sse2neon_INCLUDE_DIR
        NAMES
            sse2neon.h
        HINTS
            ${sse2neon_ROOT}
        PATH_SUFFIXES
            include
            sse2neon/include
    )

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(sse2neon_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(sse2neon
        REQUIRED_VARS 
            sse2neon_INCLUDE_DIR 
    )
    set(sse2neon_FOUND ${sse2neon_FOUND})
endif()

###############################################################################
### Configure target ###

if(sse2neon_FOUND AND NOT TARGET sse2neon)
    # INTERFACE type since we know that this is a header-only library.
    add_library(sse2neon INTERFACE IMPORTED GLOBAL)
    set(_sse2neon_TARGET_CREATE TRUE)
endif()

###############################################################################
### Configure target ###

if(_sse2neon_TARGET_CREATE)
    set_target_properties(sse2neon PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${sse2neon_INCLUDE_DIR}
    )

    mark_as_advanced(sse2neon_INCLUDE_DIR)
endif()