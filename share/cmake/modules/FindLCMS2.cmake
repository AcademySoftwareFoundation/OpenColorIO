# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install lcms2
#
# Variables defined by this module:
#   LCMS2_FOUND - If FALSE, do not try to link to lcms
#   LCMS2_LIBRARY - Where to find lcms
#   LCMS2_INCLUDE_DIR - Where to find lcms2.h
#   LCMS2_VERSION - The version of the library
#
# Targets defined by this module:
#   lcms2::lcms2 - IMPORTED target, if found
#
# By default, the dynamic libraries of LCMS2 will be found. To find the static 
# ones instead, you must set the LCMS2_STATIC_LIBRARY variable to TRUE 
# before calling find_package(LCMS2 ...).
#
# If LCMS2 is not installed in a standard path, you can use the LCMS2_DIRS 
# variable to tell CMake where to find it. If it is not found and 
# OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, LCMS2 will be 
# downloaded, built, and statically-linked into libOpenColorIO at build time.
#

if(NOT TARGET lcms2::lcms2)
    add_library(lcms2::lcms2 UNKNOWN IMPORTED GLOBAL)
    set(_LCMS2_TARGET_CREATE TRUE)
endif()

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Try to use pkg-config to get the version
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_LCMS2 QUIET lcms2)

    set(_LCMS2_SEARCH_DIRS
        ${LCMS2_DIRS}
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
    find_path(LCMS2_INCLUDE_DIR
        NAMES
            lcms2.h
        HINTS
            ${_LCMS2_SEARCH_DIRS}
            ${PC_LCMS2_INCLUDE_DIRS}
        PATH_SUFFIXES
            include
            lcms2/include
            include/lcms2
    )

    # Attempt to find static library first if this is set
    if(LCMS2_STATIC_LIBRARY)
        set(_LCMS2_STATIC "${CMAKE_STATIC_LIBRARY_PREFIX}lcms2${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endif()

    # Find library
    find_library(LCMS2_LIBRARY
        NAMES
            ${_LCMS2_STATIC} lcms2 liblcms2
        HINTS
            ${_LCMS2_SEARCH_DIRS}
            ${PC_LCMS2_LIBRARY_DIRS}
        PATH_SUFFIXES
            lib64 lib 
    )

    # Get version from config or header file
    if(LCMS2_INCLUDE_DIR AND EXISTS "${LCMS2_INCLUDE_DIR}/lcms2.h")
        file(STRINGS "${LCMS2_INCLUDE_DIR}/lcms2.h" _LCMS2_VER_SEARCH 
            REGEX "^[ \t]*//[ \t]+Version[ \t]+[.0-9]+.*$")
        if(_LCMS2_VER_SEARCH)
            string(REGEX REPLACE ".*//[ \t]+Version[ \t]+([.0-9]+).*" 
                "\\1" LCMS2_VERSION "${_LCMS2_VER_SEARCH}")
        endif()
    elseif(PC_LCMS2_FOUND)
        set(LCMS2_VERSION "${PC_LCMS2_VERSION}")
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(LCMS2_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(LCMS2
        REQUIRED_VARS 
            LCMS2_INCLUDE_DIR 
            LCMS2_LIBRARY 
        VERSION_VAR
            LCMS2_VERSION
    )
endif()

###############################################################################
### Install package from source ###

if(NOT LCMS2_FOUND)
    include(ExternalProject)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(LCMS2_FOUND TRUE)
    set(LCMS2_VERSION ${LCMS2_FIND_VERSION})
    set(LCMS2_INCLUDE_DIR "${_EXT_DIST_ROOT}/include/lcms2")
    set(LCMS2_LIBRARY 
        "${_EXT_DIST_ROOT}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}lcms2${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_LCMS2_TARGET_CREATE)
        if(UNIX)
            set(LCMS2_C_FLAGS "${LCMS2_C_FLAGS} -fPIC")
        endif()

        if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
            set(LCMS2_C_FLAGS "${LCMS2_C_FLAGS} -Wno-aggressive-loop-optimizations")
        endif()

        string(STRIP "${LCMS2_C_FLAGS}" LCMS2_C_FLAGS)

        set(LCMS2_CMAKE_ARGS
            ${LCMS2_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_SHARED_LIBS:BOOL=OFF
            -DCMAKE_C_FLAGS=${LCMS2_C_FLAGS}
        )
        if(CMAKE_TOOLCHAIN_FILE)
            set(LCMS2_CMAKE_ARGS
                ${LCMS2_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${LCMS2_INCLUDE_DIR})

        ExternalProject_Add(lcms2_install
            GIT_REPOSITORY "https://github.com/mm2/Little-CMS.git"
            GIT_TAG "lcms${LCMS2_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/Little-CMS"
            BUILD_BYPRODUCTS ${LCMS2_LIBRARY}
            PATCH_COMMAND
                ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/share/cmake/projects/BuildLCMS2.cmake"
                "CMakeLists.txt"
            CMAKE_ARGS ${LCMS2_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(lcms2::lcms2 lcms2_install)
        message(STATUS "Installing LCMS2: ${LCMS2_LIBRARY} (version ${LCMS2_VERSION})")
    endif()
endif()

###############################################################################
### Configure target ###

if(_LCMS2_TARGET_CREATE)
    set_target_properties(lcms2::lcms2 PROPERTIES
        IMPORTED_LOCATION ${LCMS2_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${LCMS2_INCLUDE_DIR}
    )

    mark_as_advanced(LCMS2_INCLUDE_DIR LCMS2_LIBRARY LCMS2_VERSION)
endif()
