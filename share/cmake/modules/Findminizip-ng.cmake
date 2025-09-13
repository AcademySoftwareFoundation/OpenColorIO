# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate minizip-ng
#
# Variables defined by this module:
#   minizip-ng_FOUND        - If FALSE, do not try to link to minizip-ng
#   minizip-ng_LIBRARY      - minizip-ng library to link to
#   minizip-ng_INCLUDE_DIR  - Where to find mz.h and other headers
#   minizip-ng_VERSION      - The version of the library
#   minizip-ng_COMPAT       - Whether minizip-ng MZ_COMPAT was used or not
#
# Global targets defined by this module:
#   MINIZIP::minizip-ng - IMPORTED target, if found
#
# Usually CMake will use the dynamic library rather than static, if both are present. 
# In this case, you may set minizip-ng_STATIC_LIBRARY to ON to request use of the static one. 
# If only the static library is present (such as when OCIO builds the dependency), then the option 
# is not needed.
#
# If the library is not installed in a typical location where CMake will find it, you may specify 
# the location using one of the following methods:
# -- Set -Dminizip-ng_DIR to point to the directory containing the CMake configuration file for the package.
# -- Set -Dminizip-ng_ROOT to point to the directory containing the lib and include directories.
# -- Set -Dminizip-ng_LIBRARY and -Dminizip-ng_INCLUDE_DIR to point to the lib and include directories.
#
#
# For external builds of minizip-ng, please note that the same build options should be used.
# Using more options, such as enabling other compression methods, will provoke linking issue
# since OCIO is not linking to those libraries.
#
# e.g. Setting MZ_BZIP2=ON will cause linking issue since OCIO will not be linked against BZIP2.
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
    if(NOT DEFINED minizip-ng_ROOT)
        # Search for minizip-ng-config.cmake
        find_package(minizip-ng ${minizip-ng_FIND_VERSION} CONFIG QUIET)
    endif()
    
    if (minizip-ng_FOUND)
        get_target_property(minizip-ng_INCLUDE_DIR MINIZIP::minizip-ng INTERFACE_INCLUDE_DIRECTORIES)
        get_target_property(minizip-ng_LIBRARY MINIZIP::minizip-ng LOCATION)
        
        # Depending on the options used when minizip-ng was built, it could have multiple libraries
        # listed in INTERFACE_LINK_LIBRARIES. OCIO only needs ZLIB.
        # Only add custom zlib target ZLIB::ZLIB to INTERFACE_LINK_LIBRARIES.
        set_target_properties(MINIZIP::minizip-ng PROPERTIES INTERFACE_LINK_LIBRARIES "ZLIB::ZLIB")

        if (NOT minizip-ng_LIBRARY)
            # Lib names to search for
            set(_minizip-ng_LIB_NAMES minizip-ng)

            if(BUILD_TYPE_DEBUG)
                # Prefer Debug lib names (Windows only)
                list(INSERT _minizip-ng_LIB_NAMES 0 minizip-ngd)
            endif()

            if(minizip-ng_STATIC_LIBRARY)
                # Prefer static lib names
                set(_minizip-ng_STATIC_LIB_NAMES 
                    "${CMAKE_STATIC_LIBRARY_PREFIX}minizip-ng${CMAKE_STATIC_LIBRARY_SUFFIX}")
                if(WIN32 AND BUILD_TYPE_DEBUG)
                    # Prefer static Debug lib names (Windows only)
                    list(INSERT _minizip-ng_STATIC_LIB_NAMES 0
                        "${CMAKE_STATIC_LIBRARY_PREFIX}minizip-ngd${CMAKE_STATIC_LIBRARY_SUFFIX}")
                endif()
            endif()

            # Find library
            find_library(minizip-ng_LIBRARY
                NAMES
                    ${_minizip-ng_STATIC_LIB_NAMES}
                    ${_minizip-ng_LIB_NAMES}
                HINTS
                    ${minizip-ng_ROOT}
                    ${PC_minizip-ng_LIBRARY_DIRS}
                PATH_SUFFIXES
                    lib64 lib 
            )

            # Set IMPORTED_LOCATION property for MINIZIP::minizip-ng target.
            if (TARGET MINIZIP::minizip-ng)
                set_target_properties(MINIZIP::minizip-ng PROPERTIES
                    IMPORTED_LOCATION "${minizip-ng_LIBRARY}"
                )
            endif()
        endif()
    else()
        list(APPEND _minizip-ng_REQUIRED_VARS minizip-ng_INCLUDE_DIR)

        # Search for minizip-ng.pc
        find_package(PkgConfig QUIET)
        pkg_check_modules(PC_minizip-ng QUIET "minizip-ng>=${minizip-ng_FIND_VERSION}")

        # Find include directory
        find_path(minizip-ng_INCLUDE_DIR
            NAMES
                mz.h
            HINTS
                ${minizip-ng_ROOT}
                ${PC_minizip-ng_INCLUDE_DIRS}
            PATH_SUFFIXES
                include
                include/minizip-ng
                include/minizip-ng/minizip-ng
                include/minizip
                minizip-ng/include
                minizip/include
        )

        # Minizip-ng uses prefix "lib" on all platform by default.
        # Library name doesn't change in debug.

        # Lib names to search for.
        # Search for "minizip" since the library could be named "minizip" if it was built 
        # with MZ_COMPAT=ON.
        set(_minizip-ng_LIB_NAMES libminizip-ng minizip-ng libminizip minizip)

        if(minizip-ng_STATIC_LIBRARY)
            # Looking for both "lib" prefix and CMAKE_STATIC_LIBRARY_PREFIX.
            # Prefer static lib names.
            set(_minizip-ng_STATIC_LIB_NAMES 
                "libminizip-ng${CMAKE_STATIC_LIBRARY_SUFFIX}"
                "${CMAKE_STATIC_LIBRARY_PREFIX}minizip-ng${CMAKE_STATIC_LIBRARY_SUFFIX}"
                "libminizip${CMAKE_STATIC_LIBRARY_SUFFIX}"
                "${CMAKE_STATIC_LIBRARY_PREFIX}minizip${CMAKE_STATIC_LIBRARY_SUFFIX}")
        endif()

        # Find library
        find_library(minizip-ng_LIBRARY
            NAMES
                ${_minizip-ng_STATIC_LIB_NAMES}
                ${_minizip-ng_LIB_NAMES}
            HINTS
                ${minizip-ng_ROOT}
                ${PC_minizip-ng_LIBRARY_DIRS}
            PATH_SUFFIXES
                lib64 lib 
        )

        # Get version from header or pkg-config
        if(minizip-ng_INCLUDE_DIR)
            list(GET minizip-ng_INCLUDE_DIR 0 _minizip-ng_INCLUDE_DIR)
            if(EXISTS "${_minizip-ng_INCLUDE_DIR}/mz.h")
                set(_minizip-ng_CONFIG "${_minizip-ng_INCLUDE_DIR}/mz.h")
            endif()
        endif()

        if(_minizip-ng_CONFIG)
            file(STRINGS "${_minizip-ng_CONFIG}" _minizip-ng_VER_SEARCH 
                REGEX "^[ \t]*#define[ \t]+MZ_VERSION[ \t]+\\(\"[.0-9]+\"\\).*$")
            if(_minizip-ng_VER_SEARCH)
                string(REGEX REPLACE ".*#define[ \t]+MZ_VERSION[ \t]+\\(\"([.0-9]+)\"\\).*" 
                    "\\1" minizip-ng_VERSION "${_minizip-ng_VER_SEARCH}")
            endif()
        elseif(PC_minizip-ng_FOUND)
            set(minizip-ng_VERSION "${PC_minizip-ng_VERSION}")
        endif()
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(minizip-ng_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(minizip-ng
        REQUIRED_VARS 
            minizip-ng_LIBRARY
            minizip-ng_INCLUDE_DIR
        VERSION_VAR
            minizip-ng_VERSION
    )
endif()

###############################################################################
### Create target

if(minizip-ng_FOUND AND NOT TARGET MINIZIP::minizip-ng)
    add_library(MINIZIP::minizip-ng UNKNOWN IMPORTED GLOBAL)
    set(_minizip-ng_TARGET_CREATE TRUE)
endif()

###############################################################################
### Configure target ###

if(_minizip-ng_TARGET_CREATE)
    set_target_properties(MINIZIP::minizip-ng PROPERTIES
        IMPORTED_LOCATION "${minizip-ng_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${minizip-ng_INCLUDE_DIR}"
    )

    mark_as_advanced(minizip-ng_INCLUDE_DIR minizip-ng_LIBRARY minizip-ng_VERSION)

    target_link_libraries(MINIZIP::minizip-ng INTERFACE ZLIB::ZLIB)
endif()

###############################################################################
### Detect compatibility mode ###

set(minizip-ng_COMPAT FALSE)
if(minizip-ng_INCLUDE_DIR)
    list(GET minizip-ng_INCLUDE_DIR 0 _minizip-ng_INCLUDE_DIR)
    if(EXISTS "${_minizip-ng_INCLUDE_DIR}/mz_compat.h")
        set(minizip-ng_COMPAT TRUE)
    endif()
endif()
mark_as_advanced(minizip-ng_COMPAT)
