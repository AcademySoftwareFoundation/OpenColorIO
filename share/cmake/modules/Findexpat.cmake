# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install expat
#
# Variables defined by this module:
#   expat_FOUND - If FALSE, do not try to link to expat
#   expat_LIBRARY - expat library to link to
#   expat_INCLUDE_DIR - Where to find expat.h
#   expat_VERSION - The version of the library
#
# Targets defined by this module:
#   expat::expat - IMPORTED target, if found
#
# By default, the dynamic libraries of expat will be found. To find the static 
# ones instead, you must set the expat_STATIC_LIBRARY variable to TRUE 
# before calling find_package(expat ...).
#
# If expat is not installed in a standard path, you can use the expat_ROOT 
# variable to tell CMake where to find it. If it is not found and 
# OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, expat will be downloaded, 
# built, and statically-linked into libOpenColorIO at build time.
#

###############################################################################
### Try to find package ###

# BUILD_TYPE_DEBUG variable is currently set in one of the OCIO's CMake files. 
# Now that some OCIO's find module are installed with the library itself (with static build), 
# a consumer app don't have access to the variables set by an OCIO's CMake files. Therefore, some 
# OCIO's find modules must detect the build type by itselves. 
set(BUILD_TYPE_DEBUG OFF)
if(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
   set(BUILD_TYPE_DEBUG ON)
endif()

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_expat_REQUIRED_VARS expat_LIBRARY)

    if(NOT DEFINED expat_ROOT)
        # Search for expat-config.cmake
        find_package(expat ${expat_FIND_VERSION} CONFIG QUIET)
    endif()

    if(expat_FOUND)
        if (TARGET expat::libexpat)
            message(STATUS "Expat ${expat_VERSION} detected, aliasing targets.")
            add_library(expat::expat ALIAS expat::libexpat)
        endif()

        get_target_property(expat_INCLUDE_DIR expat::expat INTERFACE_INCLUDE_DIRECTORIES)

        get_target_property(expat_LIBRARY expat::expat LOCATION)

        if (NOT expat_INCLUDE_DIR)
            # Find include directory too, as its Config module doesn't include it
            find_path(expat_INCLUDE_DIR
                NAMES
                    expat.h
                HINTS
                    ${expat_ROOT}
                    ${PC_expat_INCLUDE_DIRS}
                PATH_SUFFIXES
                    include
                    expat/include
            )
            message(WARNING "Expat's include directory not specified in its Config module, patching it now to ${expat_INCLUDE_DIR}.")
            if (TARGET expat::libexpat)
                set_target_properties(expat::libexpat PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES ${expat_INCLUDE_DIR}
                )
            else()
                set_target_properties(expat::expat PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES ${expat_INCLUDE_DIR}
                )
            endif()
        endif()

        set(_expat_REQUIRED_VARS ${expat_REQUIRED_VARS} expat_INCLUDE_DIR)
    else()
        list(APPEND _expat_REQUIRED_VARS expat_INCLUDE_DIR)

        # Search for expat.pc
        find_package(PkgConfig QUIET)
        pkg_check_modules(PC_expat QUIET "expat>=${expat_FIND_VERSION}")

        # Find include directory
        find_path(expat_INCLUDE_DIR
            NAMES
                expat.h
            HINTS
                ${expat_ROOT}
                ${PC_expat_INCLUDE_DIRS}
            PATH_SUFFIXES
                include
                expat/include
        )

        # Expat uses prefix "lib" on all platform by default.
        # Library name doesn't change in debug.

        # For Windows, see https://github.com/libexpat/libexpat/blob/R_2_2_8/expat/win32/README.txt.
        # libexpat<postfix=[w][d][MD|MT]>.lib
        # The "w" indicates the UTF-16 version of the library.

        # Lib names to search for
        set(_expat_LIB_NAMES libexpat expat)

        if(WIN32 AND BUILD_TYPE_DEBUG)
            # Prefer Debug lib names. The library name changes only on Windows.
            list(INSERT _expat_LIB_NAMES 0 libexpatd libexpatwd expatd expatwd)
        elseif(WIN32)
            # libexpat(w).dll/.lib
            list(APPEND _expat_LIB_NAMES libexpatw expatw)
        endif()

        if(expat_STATIC_LIBRARY)
            # Looking for both "lib" prefix and CMAKE_STATIC_LIBRARY_PREFIX.
            # Prefer static lib names
            set(_expat_STATIC_LIB_NAMES
                "libexpat${CMAKE_STATIC_LIBRARY_SUFFIX}"
                "${CMAKE_STATIC_LIBRARY_PREFIX}expat${CMAKE_STATIC_LIBRARY_SUFFIX}")

            if(WIN32 AND BUILD_TYPE_DEBUG)
                # Prefer static Debug lib names. The library name changes only on Windows.
                list(INSERT _expat_STATIC_LIB_NAMES 0
                    "libexpatdMD${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "libexpatdMT${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "libexpatd${CMAKE_STATIC_LIBRARY_SUFFIX}"
                    "libexpatwdMD${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "libexpatwdMT${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "libexpatwd${CMAKE_STATIC_LIBRARY_SUFFIX}"
                    "${CMAKE_STATIC_LIBRARY_PREFIX}expatdMD${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "${CMAKE_STATIC_LIBRARY_PREFIX}expatdMT${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "${CMAKE_STATIC_LIBRARY_PREFIX}expatd${CMAKE_STATIC_LIBRARY_SUFFIX}"
                    "${CMAKE_STATIC_LIBRARY_PREFIX}expatwdMD${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "${CMAKE_STATIC_LIBRARY_PREFIX}expatwdMT${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "${CMAKE_STATIC_LIBRARY_PREFIX}expatwd${CMAKE_STATIC_LIBRARY_SUFFIX}")
            elseif (WIN32)
                list(INSERT _expat_STATIC_LIB_NAMES 0
                    "libexpatMD${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "libexpatMT${CMAKE_STATIC_LIBRARY_SUFFIX}"
                    "libexpatwMD${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "libexpatwMT${CMAKE_STATIC_LIBRARY_SUFFIX}"
                    "${CMAKE_STATIC_LIBRARY_PREFIX}expatMD${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "${CMAKE_STATIC_LIBRARY_PREFIX}expatMT${CMAKE_STATIC_LIBRARY_SUFFIX}"
                    "${CMAKE_STATIC_LIBRARY_PREFIX}expatwMD${CMAKE_STATIC_LIBRARY_SUFFIX}" 
                    "${CMAKE_STATIC_LIBRARY_PREFIX}expatwMT${CMAKE_STATIC_LIBRARY_SUFFIX}"
                    )
            endif()
        endif()

        # Find library
        find_library(expat_LIBRARY
            NAMES
                ${_expat_STATIC_LIB_NAMES}
                ${_expat_LIB_NAMES}
            HINTS
                ${expat_ROOT}
                ${PC_expat_LIBRARY_DIRS}
            PATH_SUFFIXES
                lib64 lib 
        )

        # Get version from header or pkg-config
        if(expat_INCLUDE_DIR AND EXISTS "${expat_INCLUDE_DIR}/expat.h")
            file(STRINGS "${expat_INCLUDE_DIR}/expat.h" _expat_VER_SEARCH 
                REGEX "^[ \t]*#define[ \t]+XML_(MAJOR|MINOR|MICRO)_VERSION[ \t]+[0-9]+.*$")
            if(_expat_VER_SEARCH)
                foreach(_VER_PART MAJOR MINOR MICRO)
                    string(REGEX REPLACE ".*#define[ \t]+XML_${_VER_PART}_VERSION[ \t]+([0-9]+).*" 
                        "\\1" expat_VERSION_${_VER_PART} "${_expat_VER_SEARCH}")
                    if(NOT expat_VERSION_${_VER_PART})
                        set(expat_VERSION_${_VER_PART} 0)
                    endif()
                endforeach()
                set(expat_VERSION 
                    "${expat_VERSION_MAJOR}.${expat_VERSION_MINOR}.${expat_VERSION_MICRO}")
            endif()
        elseif(PC_expat_FOUND)
            set(expat_VERSION "${PC_expat_VERSION}")
        endif()
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(expat_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(expat
        REQUIRED_VARS 
            ${_expat_REQUIRED_VARS}
        VERSION_VAR
            expat_VERSION
    )
endif()

###############################################################################
### Create target

if(expat_FOUND AND NOT TARGET expat::expat)
    add_library(expat::expat UNKNOWN IMPORTED GLOBAL)
    set(_expat_TARGET_CREATE TRUE)
endif()

###############################################################################
### Configure target ###

if(_expat_TARGET_CREATE)
    set_target_properties(expat::expat PROPERTIES
        IMPORTED_LOCATION ${expat_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${expat_INCLUDE_DIR}
    )

    mark_as_advanced(expat_INCLUDE_DIR expat_LIBRARY expat_VERSION)
endif()