# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate lcms2
#
# Variables defined by this module:
#   lcms2_FOUND - If FALSE, do not try to link to lcms
#   lcms2_LIBRARY - Where to find lcms
#   lcms2_INCLUDE_DIR - Where to find lcms2.h
#   lcms2_VERSION - The version of the library
#
# Global targets defined by this module:
#   lcms2::lcms2 - IMPORTED target, if found
#
# Usually CMake will use the dynamic library rather than static, if both are present. 
# In this case, you may set lcms2_STATIC_LIBRARY to ON to request use of the static one. 
# If only the static library is present (such as when OCIO builds the dependency), then the option 
# is not needed.
#
# If the library is not installed in a typical location where CMake will find it, you may specify 
# the location using one of the following methods:
# -- Set -Dlcms2_ROOT to point to the directory containing the lib and include directories.
# -- Set -Dlcms2_LIBRARY and -Dlcms2_INCLUDE_DIR to point to the lib and include directories.
#

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Try to use pkg-config to get the version
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_lcms2 QUIET "lcms2>=${lcms2_FIND_VERSION}")

    # Find include directory
    find_path(lcms2_INCLUDE_DIR
        NAMES
            lcms2.h
        HINTS
            ${lcms2_ROOT}
            ${PC_lcms2_INCLUDE_DIRS}
        PATH_SUFFIXES
            include
            lcms2/include
            lcms2/include/lcms2
            include/lcms2
    )

    # Attempt to find static library first if this is set
    if(lcms2_STATIC_LIBRARY)
        set(_lcms2_STATIC "${CMAKE_STATIC_LIBRARY_PREFIX}lcms2${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endif()

    # Find library
    find_library(lcms2_LIBRARY
        NAMES
            ${_lcms2_STATIC} lcms2 liblcms2
        HINTS
            ${lcms2_ROOT}
            ${PC_lcms2_LIBRARY_DIRS}
        PATH_SUFFIXES
            lcms2/lib
            lib64
            lib
    )

    # Get version from config or header file
    if(lcms2_INCLUDE_DIR AND EXISTS "${lcms2_INCLUDE_DIR}/lcms2.h")
        file(STRINGS "${lcms2_INCLUDE_DIR}/lcms2.h" _lcms2_VER_SEARCH 
            REGEX "^[ \t]*//[ \t]+Version[ \t]+[.0-9]+.*$")
        if(_lcms2_VER_SEARCH)
            string(REGEX REPLACE ".*//[ \t]+Version[ \t]+([.0-9]+).*" 
                "\\1" lcms2_VERSION "${_lcms2_VER_SEARCH}")
        endif()
    elseif(PC_lcms2_FOUND)
        set(lcms2_VERSION "${PC_lcms2_VERSION}")
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(lcms2_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(lcms2
        REQUIRED_VARS 
            lcms2_INCLUDE_DIR 
            lcms2_LIBRARY 
        VERSION_VAR
            lcms2_VERSION
    )
endif()

###############################################################################
### Configure target ###

if(lcms2_FOUND AND NOT TARGET lcms2::lcms2)
    add_library(lcms2::lcms2 UNKNOWN IMPORTED GLOBAL)
    set(_lcms2_TARGET_CREATE TRUE)
endif()

###############################################################################
### Configure target ###

if(_lcms2_TARGET_CREATE)
    set_target_properties(lcms2::lcms2 PROPERTIES
        IMPORTED_LOCATION ${lcms2_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${lcms2_INCLUDE_DIR}
    )

    mark_as_advanced(lcms2_INCLUDE_DIR lcms2_LIBRARY lcms2_VERSION)
endif()
