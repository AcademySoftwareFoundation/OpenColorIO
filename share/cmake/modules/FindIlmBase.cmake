# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install ilmbase
#
# Variables defined by this module:
#   ILMBASE_FOUND - If FALSE, do not try to link to ilmbase
#   ILMBASE_LIBRARY - Where to find Half
#   ILMBASE_INCLUDE_DIR - Where to find half.h
#   ILMBASE_VERSION - The version of the library
#
# Targets defined by this module:
#   ilmbase::ilmbase - IMPORTED target, if found
#
# By default, the dynamic libraries of ilmbase will be found. To find the 
# static ones instead, you must set the ILMBASE_STATIC_LIBRARY variable to 
# TRUE before calling find_package(IlmBase ...).
#
# If ilmbase is not installed in a standard path, you can use the 
# ILMBASE_DIRS variable to tell CMake where to find it. If it is not found 
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, ilmbase will be 
# downloaded, built, and statically-linked into libOpenColorIO at build time.
#

if (NOT TARGET ilmbase::ilmbase)
    add_library(ilmbase::ilmbase UNKNOWN IMPORTED GLOBAL)
    set(_ILMBASE_TARGET_CREATE TRUE)
endif()

# IlmBase components may have the version in their name
set(_ILMBASE_LIB_VER "${IlmBase_FIND_VERSION_MAJOR}_${IlmBase_FIND_VERSION_MINOR}")

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_ILMBASE_SEARCH_DIRS
        ${ILMBASE_DIRS}
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
    find_path(ILMBASE_INCLUDE_DIR
        NAMES
            OpenEXR/half.h
        HINTS
            ${_ILMBASE_SEARCH_DIRS}
        PATH_SUFFIXES
            include
            OpenEXR/include
    )

    # Lib names to search for
    set(_ILMBASE_LIB_NAMES "Half-${_ILMBASE_LIB_VER}" Half)
    if(BUILD_TYPE_DEBUG)
        # Prefer Debug lib names
        list(INSERT _ILMBASE_LIB_NAMES 0 "Half-${_ILMBASE_LIB_VER}_d")
    endif()

    if(ILMBASE_STATIC_LIBRARY)
        # Prefer static lib names
        set(_ILMBASE_STATIC_LIB_NAMES 
            "${CMAKE_STATIC_LIBRARY_PREFIX}Half-${_ILMBASE_LIB_VER}_s${CMAKE_STATIC_LIBRARY_SUFFIX}"
            "${CMAKE_STATIC_LIBRARY_PREFIX}Half${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )
        if(BUILD_TYPE_DEBUG)
            # Prefer static Debug lib names
            list(INSERT _ILMBASE_STATIC_LIB_NAMES 0
                "${CMAKE_STATIC_LIBRARY_PREFIX}Half-${_ILMBASE_LIB_VER}_s_d${CMAKE_STATIC_LIBRARY_SUFFIX}")
        endif()
    endif()

    # Find library
    find_library(ILMBASE_LIBRARY
        NAMES
            ${_ILMBASE_STATIC_LIB_NAMES} 
            ${_ILMBASE_LIB_NAMES}
        HINTS
            ${_ILMBASE_SEARCH_DIRS}
        PATH_SUFFIXES
            lib64 lib 
    )

    # Get version from config header file
    if(ILMBASE_INCLUDE_DIR)
        if(EXISTS "${ILMBASE_INCLUDE_DIR}/OpenEXR/IlmBaseConfig.h")
            set(_ILMBASE_CONFIG "${ILMBASE_INCLUDE_DIR}/OpenEXR/IlmBaseConfig.h")
        elseif(EXISTS "${ILMBASE_INCLUDE_DIR}/OpenEXR/OpenEXRConfig.h")
            set(_ILMBASE_CONFIG "${ILMBASE_INCLUDE_DIR}/OpenEXR/OpenEXRConfig.h")
        endif()
    endif()

    if(_ILMBASE_CONFIG)
        file(STRINGS "${_ILMBASE_CONFIG}" _ILMBASE_VER_SEARCH 
            REGEX "^[ \t]*#define[ \t]+(OPENEXR|ILMBASE)_VERSION_STRING[ \t]+\"[.0-9]+\".*$")
        if(_ILMBASE_VER_SEARCH)
            string(REGEX REPLACE ".*#define[ \t]+(OPENEXR|ILMBASE)_VERSION_STRING[ \t]+\"([.0-9]+)\".*" 
                "\\2" ILMBASE_VERSION "${_ILMBASE_VER_SEARCH}")
        endif()
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(IlmBase_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(IlmBase
        REQUIRED_VARS 
            ILMBASE_INCLUDE_DIR 
            ILMBASE_LIBRARY 
        VERSION_VAR
            ILMBASE_VERSION
    )
    set(ILMBASE_FOUND ${IlmBase_FOUND})
endif()

###############################################################################
### Install package from source ###

if(NOT ILMBASE_FOUND)
    include(ExternalProject)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(ILMBASE_FOUND TRUE)
    set(ILMBASE_VERSION ${IlmBase_FIND_VERSION})
    set(ILMBASE_INCLUDE_DIR "${_EXT_DIST_ROOT}/include")

    # Set the expected library name. "_d" is appended to Debug Windows builds 
    # <= OpenEXR 2.3.0. In newer versions, it is appended to Debug libs on
    # all platforms.
    if(BUILD_TYPE_DEBUG AND (WIN32 OR ILMBASE_VERSION VERSION_GREATER "2.3.0"))
        set(_ILMBASE_LIB_SUFFIX "_d")
    endif()

    set(ILMBASE_LIBRARY
        "${_EXT_DIST_ROOT}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}Half-${_ILMBASE_LIB_VER}_s${_ILMBASE_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_ILMBASE_TARGET_CREATE)
        if(UNIX)
            set(ILMBASE_CXX_FLAGS "${ILMBASE_CXX_FLAGS} -fPIC")
        endif()

        if(MSVC)
            set(ILMBASE_CXX_FLAGS "${ILMBASE_CXX_FLAGS} /EHsc")
        endif()

        string(STRIP "${ILMBASE_CXX_FLAGS}" ILMBASE_CXX_FLAGS)

        set(ILMBASE_CMAKE_ARGS
            ${ILMBASE_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DOPENEXR_BUILD_ILMBASE:BOOL=ON
            -DOPENEXR_BUILD_OPENEXR:BOOL=OFF
            -DOPENEXR_BUILD_PYTHON_LIBS:BOOL=OFF
            -DOPENEXR_BUILD_SHARED:BOOL=OFF
            -DOPENEXR_BUILD_STATIC:BOOL=ON
            -DOPENEXR_BUILD_UTILS:BOOL=OFF
            -DOPENEXR_BUILD_TESTS:BOOL=OFF
            -DCMAKE_CXX_FLAGS=${ILMBASE_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(ILMBASE_CMAKE_ARGS
                ${ILMBASE_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${ILMBASE_INCLUDE_DIR})

        ExternalProject_Add(ilmbase_install
            GIT_REPOSITORY "https://github.com/openexr/openexr.git"
            GIT_TAG "v${ILMBASE_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/openexr"
            BUILD_BYPRODUCTS ${ILMBASE_LIBRARY}
            CMAKE_ARGS ${ILMBASE_CMAKE_ARGS}
            BUILD_COMMAND
                ${CMAKE_COMMAND} --build .
                                 --config ${CMAKE_BUILD_TYPE}
                                 --target Half_static
            INSTALL_COMMAND
                ${CMAKE_COMMAND} -DCMAKE_INSTALL_CONFIG_NAME=${CMAKE_BUILD_TYPE}
                                 -P "IlmBase/Half/cmake_install.cmake"
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(ilmbase::ilmbase ilmbase_install)
        message(STATUS "Installing IlmBase: ${ILMBASE_LIBRARY} (version ${ILMBASE_VERSION})")
    endif()
endif()

###############################################################################
### Configure target ###

if(_ILMBASE_TARGET_CREATE)
    set_target_properties(ilmbase::ilmbase PROPERTIES
        IMPORTED_LOCATION ${ILMBASE_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${ILMBASE_INCLUDE_DIR}
    )

    mark_as_advanced(ILMBASE_INCLUDE_DIR ILMBASE_LIBRARY ILMBASE_VERSION)
endif()
