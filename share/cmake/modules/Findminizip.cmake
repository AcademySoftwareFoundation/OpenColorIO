# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate minizip-ng with MZ_COMPAT=ON.
# 
# This module DOES NOT install minizip-ng with MZ_COMPAT=ON if it is not found.
#
# Note: This option changes the name for the library file to "libminizip", but it is still
#       actually minizip-ng. OCIO uses the API from minizip-ng.
#
# Variables defined by this module:
#   minizip_FOUND        - If FALSE, do not try to link to minizip
#   minizip_LIBRARY      - minizip library to link to
#   minizip_INCLUDE_DIR  - Where to find mz.h and other headers
#   minizip_VERSION      - The version of the library
#
#   This module set the variables below because this is still minizip-ng. The librarie become
#   "minizip" because of the cmake option MZ_COMPAT=ON.
#
#   minizip-ng_FOUND        - If FALSE, do not try to link to minizip-ng
#   minizip-ng_LIBRARY      - minizip-ng library to link to
#   minizip-ng_INCLUDE_DIR  - Where to find mz.h and other headers
#   minizip-ng_VERSION      - The version of the library
#
# Targets defined by this module:
#   MINIZIP::minizip-ng - IMPORTED target, if found
#
###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)

    if(NOT DEFINED minizip_ROOT)
        # Search for minizip-config.cmake
        find_package(minizip ${minizip_FIND_VERSION} CONFIG QUIET)
    endif()

    if (minizip_FOUND)
        get_target_property(minizip_INCLUDE_DIR MINIZIP::minizip INTERFACE_INCLUDE_DIRECTORIES)
        get_target_property(minizip_LIBRARY MINIZIP::minizip LOCATION)

        if (NOT minizip_LIBRARY)
            # Lib names to search for
            set(_minizip_LIB_NAMES minizip)

            if(BUILD_TYPE_DEBUG)
                # Prefer Debug lib names (Windows only)
                list(INSERT _minizip_LIB_NAMES 0 minizipd)
            endif()

            if(minizip_STATIC_LIBRARY)
                # Prefer static lib names
                set(_minizip_STATIC_LIB_NAMES 
                    "${CMAKE_STATIC_LIBRARY_PREFIX}minizip${CMAKE_STATIC_LIBRARY_SUFFIX}")
                if(WIN32 AND BUILD_TYPE_DEBUG)
                    # Prefer static Debug lib names (Windows only)
                    list(INSERT _minizip_STATIC_LIB_NAMES 0
                        "${CMAKE_STATIC_LIBRARY_PREFIX}minizipd${CMAKE_STATIC_LIBRARY_SUFFIX}")
                endif()
            endif()

            # Find library
            find_library(minizip_LIBRARY
                NAMES
                    ${_minizip_STATIC_LIB_NAMES}
                    ${_minizip_LIB_NAMES}
                HINTS
                    ${minizip_ROOT}
                    ${PC_minizip_LIBRARY_DIRS}
                PATH_SUFFIXES
                    lib64 lib 
            )
        endif()
    else()
        list(APPEND _minizip_REQUIRED_VARS minizip_INCLUDE_DIR)

        # Search for minizip.pc
        find_package(PkgConfig QUIET)
        pkg_check_modules(PC_minizip QUIET "minizip>=${minizip_FIND_VERSION}")

        # Find include directory
        find_path(minizip_INCLUDE_DIR
            NAMES
                mz.h
            HINTS
                ${minizip_ROOT}
                ${PC_minizip_INCLUDE_DIRS}
            PATH_SUFFIXES
                include
                minizip/include
        )

        # Lib names to search for
        set(_minizip_LIB_NAMES minizip)

        if(BUILD_TYPE_DEBUG)
            # Prefer Debug lib names (Windows only)
            list(INSERT _minizip_LIB_NAMES 0 minizipd)
        endif()

        if(minizip_STATIC_LIBRARY)
            # Prefer static lib names
            set(_minizip_STATIC_LIB_NAMES 
                "${CMAKE_STATIC_LIBRARY_PREFIX}minizip${CMAKE_STATIC_LIBRARY_SUFFIX}")
            if(WIN32 AND BUILD_TYPE_DEBUG)
                # Prefer static Debug lib names (Windows only)
                list(INSERT _minizip_STATIC_LIB_NAMES 0
                    "${CMAKE_STATIC_LIBRARY_PREFIX}minizipd${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endif()
        endif()

        # Find library
        find_library(minizip_LIBRARY
            NAMES
                ${_minizip_STATIC_LIB_NAMES}
                ${_minizip_LIB_NAMES}
            HINTS
                ${minizip_ROOT}
                ${PC_minizip_LIBRARY_DIRS}
            PATH_SUFFIXES
                lib64
                lib 
        )

        # Get version from header or pkg-config
        if(minizip_INCLUDE_DIR)
            list(GET minizip_INCLUDE_DIR 0 _minizip_INCLUDE_DIR)
            if(EXISTS "${_minizip_INCLUDE_DIR}/mz.h")
                set(_minizip_CONFIG "${_minizip_INCLUDE_DIR}/mz.h")
            endif()
        endif()

        if(_minizip_CONFIG)
            file(STRINGS "${_minizip_CONFIG}" _minizip_VER_SEARCH 
                REGEX "^[ \t]*#define[ \t]+MZ_VERSION[ \t]+\\(\"[.0-9]+\"\\).*$")
            if(_minizip_VER_SEARCH)
                string(REGEX REPLACE ".*#define[ \t]+MZ_VERSION[ \t]+\\(\"([.0-9]+)\"\\).*" 
                    "\\1" minizip_VERSION "${_minizip_VER_SEARCH}")
            endif()
        elseif(PC_minizip_FOUND)
            set(minizip_VERSION "${PC_minizip_VERSION}")
        endif()
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(minizip_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(minizip
        REQUIRED_VARS 
            minizip_LIBRARY
            minizip_INCLUDE_DIR
        VERSION_VAR
            minizip_VERSION
    )
endif()

###############################################################################
### Create target

if(minizip_FOUND AND NOT TARGET MINIZIP::minizip-ng)
    add_library(MINIZIP::minizip-ng UNKNOWN IMPORTED GLOBAL)
    set(_minizip_TARGET_CREATE TRUE)
endif()

###############################################################################

###############################################################################
### Configure target ###

if(minizip_FOUND AND _minizip_TARGET_CREATE)
    set_target_properties(MINIZIP::minizip-ng PROPERTIES
        IMPORTED_LOCATION "${minizip_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${minizip_INCLUDE_DIR}"
    )

    # This is still minizip-ng even though the library is called minizip because of MZ_COMPAT=ON.
    set(minizip-ng_LIBRARY ${minizip_LIBRARY})
    set(minizip-ng_INCLUDE_DIR ${minizip_INCLUDE_DIR})
    set(minizip-ng_FOUND ${minizip_FOUND})
    set(minizip-ng_VERSION ${minizip_VERSION})

    mark_as_advanced(minizip_INCLUDE_DIR minizip_LIBRARY minizip_VERSION)

    target_link_libraries(MINIZIP::minizip-ng INTERFACE ZLIB::ZLIB)
endif()