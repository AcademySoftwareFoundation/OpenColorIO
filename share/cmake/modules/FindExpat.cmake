# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install expat
#
# Variables defined by this module:
#   EXPAT_FOUND - If FALSE, do not try to link to expat
#   EXPAT_LIBRARY - Where to find expat
#   EXPAT_INCLUDE_DIR - Where to find expat.h
#   EXPAT_VERSION - The version of the library
#
# Targets defined by this module:
#   expat::expat - IMPORTED target, if found
#
# By default, the dynamic libraries of expat will be found. To find the static 
# ones instead, you must set the EXPAT_STATIC_LIBRARY variable to TRUE 
# before calling find_package(Expat ...).
#
# If expat is not installed in a standard path, you can use the EXPAT_DIRS 
# variable to tell CMake where to find it. If it is not found and 
# OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, expat will be downloaded, 
# built, and statically-linked into libOpenColorIO at build time.
#

if(NOT TARGET expat::expat)
    add_library(expat::expat UNKNOWN IMPORTED GLOBAL)
    set(_EXPAT_TARGET_CREATE TRUE)
endif()

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Try to use pkg-config to get the version
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_EXPAT QUIET expat)

    set(_EXPAT_SEARCH_DIRS
        ${EXPAT_DIRS}
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw        # Fink
        /opt/local # DarwinPorts
        /opt/csw   # Blastwave
        /opt
    )

    # Find include directory
    find_path(EXPAT_INCLUDE_DIR
        NAMES
            expat.h
        HINTS
            ${_EXPAT_SEARCH_DIRS}
            ${PC_EXPAT_INCLUDE_DIRS}
        PATH_SUFFIXES
            include
            expat/include
    )

    # Lib names to search for
    set(_EXPAT_LIB_NAMES expat libexpat)
    if(WIN32 AND BUILD_TYPE_DEBUG)
        # Prefer Debug lib names (Windows only)
        list(INSERT _EXPAT_LIB_NAMES 0 expatd)
    endif()

    if(EXPAT_STATIC_LIBRARY)
        # Prefer static lib names
        set(_EXPAT_STATIC_LIB_NAMES 
            "${CMAKE_STATIC_LIBRARY_PREFIX}expat${CMAKE_STATIC_LIBRARY_SUFFIX}")
        if(WIN32 AND BUILD_TYPE_DEBUG)
            # Prefer static Debug lib names (Windows only)
            list(INSERT _EXPAT_STATIC_LIB_NAMES 0
                "${CMAKE_STATIC_LIBRARY_PREFIX}expatd${CMAKE_STATIC_LIBRARY_SUFFIX}")
        endif()
    endif()

    # Find library
    find_library(EXPAT_LIBRARY
        NAMES
            ${_EXPAT_STATIC_LIB_NAMES}
            ${_EXPAT_LIB_NAMES}
        HINTS
            ${_EXPAT_SEARCH_DIRS}
            ${PC_EXPAT_LIBRARY_DIRS}
        PATH_SUFFIXES
            lib64 lib 
    )

    # Get version from header or config file
    # if(PC_EXPAT_FOUND)
    #     set(EXPAT_VERSION "${PC_EXPAT_VERSION}")
    if(EXPAT_INCLUDE_DIR AND EXISTS "${EXPAT_INCLUDE_DIR}/expat.h")
        file(STRINGS "${EXPAT_INCLUDE_DIR}/expat.h" _EXPAT_VER_SEARCH 
            REGEX "^[ \t]*#define[ \t]+XML_(MAJOR|MINOR|MICRO)_VERSION[ \t]+[0-9]+.*$")
        if(_EXPAT_VER_SEARCH)
            foreach(_VER_PART MAJOR MINOR MICRO)
                string(REGEX REPLACE ".*#define[ \t]+XML_${_VER_PART}_VERSION[ \t]+([0-9]+).*" 
                    "\\1" EXPAT_VERSION_${_VER_PART} "${_EXPAT_VER_SEARCH}")
                if(NOT EXPAT_VERSION_${_VER_PART})
                    set(EXPAT_VERSION_${_VER_PART} 0)
                endif()
            endforeach()
            set(EXPAT_VERSION 
                "${EXPAT_VERSION_MAJOR}.${EXPAT_VERSION_MINOR}.${EXPAT_VERSION_MICRO}")
        endif()
    elseif(PC_EXPAT_FOUND)
        set(EXPAT_VERSION "${PC_EXPAT_VERSION}")
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(Expat_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Expat
        REQUIRED_VARS 
            EXPAT_INCLUDE_DIR 
            EXPAT_LIBRARY 
        VERSION_VAR
            EXPAT_VERSION
    )
    set(EXPAT_FOUND ${Expat_FOUND})
endif()

###############################################################################
### Install package from source ###

if(NOT EXPAT_FOUND)
    include(ExternalProject)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(EXPAT_FOUND TRUE)
    set(EXPAT_VERSION ${Expat_FIND_VERSION})
    set(EXPAT_INCLUDE_DIR "${_EXT_DIST_ROOT}/include")

    # Set the expected library name
    if(WIN32 AND BUILD_TYPE_DEBUG)
        set(_EXPAT_LIB_SUFFIX "d")
    endif()
    set(EXPAT_LIBRARY
        "${_EXT_DIST_ROOT}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}expat${_EXPAT_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_EXPAT_TARGET_CREATE)
        if(UNIX)
            set(EXPAT_C_FLAGS "${EXPAT_C_FLAGS} -fPIC")
            set(EXPAT_CXX_FLAGS "${EXPAT_CXX_FLAGS} -fPIC")
        endif()

        if(MSVC)
            set(EXPAT_C_FLAGS "${EXPAT_C_FLAGS} /EHsc")
            set(EXPAT_CXX_FLAGS "${EXPAT_CXX_FLAGS} /EHsc")
        endif()

        string(STRIP "${EXPAT_C_FLAGS}" EXPAT_C_FLAGS)
        string(STRIP "${EXPAT_CXX_FLAGS}" EXPAT_CXX_FLAGS)

        set(EXPAT_CMAKE_ARGS
            ${EXPAT_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_examples:BOOL=OFF
            -DBUILD_tests:BOOL=OFF
            -DBUILD_shared:BOOL=OFF
            -DBUILD_doc:BOOL=OFF
            -DCMAKE_C_FLAGS=${EXPAT_C_FLAGS}
            -DCMAKE_CXX_FLAGS=${EXPAT_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        )
        if(CMAKE_TOOLCHAIN_FILE)
            set(EXPAT_CMAKE_ARGS
                ${EXPAT_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${EXPAT_INCLUDE_DIR})

        ExternalProject_Add(expat_install
            GIT_REPOSITORY "https://github.com/libexpat/libexpat.git"
            GIT_TAG "R_${Expat_FIND_VERSION_MAJOR}_${Expat_FIND_VERSION_MINOR}_${Expat_FIND_VERSION_PATCH}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/libexpat"
            BUILD_BYPRODUCTS ${EXPAT_LIBRARY}
            SOURCE_SUBDIR expat
            CMAKE_ARGS ${EXPAT_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(expat::expat expat_install)
        message(STATUS "Installing Expat: ${EXPAT_LIBRARY} (version ${EXPAT_VERSION})")
    endif()
endif()

###############################################################################
### Configure target ###

if(_EXPAT_TARGET_CREATE)
    set_target_properties(expat::expat PROPERTIES
        IMPORTED_LOCATION ${EXPAT_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${EXPAT_INCLUDE_DIR}
    )

    mark_as_advanced(EXPAT_INCLUDE_DIR EXPAT_LIBRARY EXPAT_VERSION)
endif()
