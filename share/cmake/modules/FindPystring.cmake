# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install pystring
#
# Variables defined by this module:
#   PYSTRING_FOUND - If FALSE, do not try to link to pystring
#   PYSTRING_LIBRARY - Where to find pystring
#   PYSTRING_INCLUDE_DIR - Where to find pystring.h
#
# Targets defined by this module:
#   pystring::pystring - IMPORTED target, if found
#
# By default, the dynamic libraries of pystring will be found. To find the 
# static ones instead, you must set the PYSTRING_STATIC_LIBRARY variable to 
# TRUE before calling find_package(Pystring ...).
#
# If pystring is not installed in a standard path, you can use the 
# PYSTRING_DIRS variable to tell CMake where to find it. If it is not found 
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, pystring will be 
# downloaded, built, and statically-linked into libOpenColorIO at build time.
#

if(NOT TARGET pystring::pystring)
    add_library(pystring::pystring UNKNOWN IMPORTED GLOBAL)
    set(_PYSTRING_TARGET_CREATE TRUE)
endif()

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_PYSTRING_SEARCH_DIRS
        ${PYSTRING_DIRS}
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
    find_path(PYSTRING_INCLUDE_DIR
        NAMES
            pystring/pystring.h
        HINTS
            ${_PYSTRING_SEARCH_DIRS}
        PATH_SUFFIXES
            include
            pystring/include
    )

    # Attempt to find static library first if this is set
    if(PYSTRING_STATIC_LIBRARY)
        set(_PYSTRING_STATIC 
            "${CMAKE_STATIC_LIBRARY_PREFIX}pystring${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endif()

    # Find library
    find_library(PYSTRING_LIBRARY
        NAMES
            ${_PYSTRING_STATIC} pystring
        HINTS
            ${_PYSTRING_SEARCH_DIRS}
        PATH_SUFFIXES
            lib64 lib 
    )

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(Pystring_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Pystring
        REQUIRED_VARS 
            PYSTRING_INCLUDE_DIR 
            PYSTRING_LIBRARY
    )
    set(PYSTRING_FOUND ${Pystring_FOUND})
endif()

###############################################################################
### Install package from source ###

if(NOT PYSTRING_FOUND)
    include(ExternalProject)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(PYSTRING_FOUND TRUE)
    set(PYSTRING_VERSION ${Pystring_FIND_VERSION})
    set(PYSTRING_INCLUDE_DIR "${_EXT_DIST_ROOT}/include")
    set(PYSTRING_LIBRARY 
        "${_EXT_DIST_ROOT}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}pystring${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_PYSTRING_TARGET_CREATE)
        if(UNIX)
            set(PYSTRING_CXX_FLAGS "${PYSTRING_CXX_FLAGS} -fvisibility=hidden -fPIC")
            if(OCIO_INLINES_HIDDEN)
                set(PYSTRING_CXX_FLAGS "${PYSTRING_CXX_FLAGS} -fvisibility-inlines-hidden")
            endif()
        endif()

        if(MSVC)
            set(PYSTRING_CXX_FLAGS "${PYSTRING_CXX_FLAGS} /EHsc")
        endif()

        string(STRIP "${PYSTRING_CXX_FLAGS}" PYSTRING_CXX_FLAGS)

        set(PYSTRING_CMAKE_ARGS
            ${PYSTRING_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${PYSTRING_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        )
        if(CMAKE_TOOLCHAIN_FILE)
            set(PYSTRING_CMAKE_ARGS
                ${PYSTRING_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(NOT BUILD_SHARED_LIBS)
            #TODO: Find a way to merge in the static libs when built with internal pystring
            message(WARNING
                "Building STATIC libOpenColorIO using the in-built Pystring. "
                "Pystring symbols are NOT included in the output binary!"
            )
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${PYSTRING_INCLUDE_DIR})

        ExternalProject_Add(pystring_install
            GIT_REPOSITORY "https://github.com/imageworks/pystring.git"
            GIT_TAG "v${Pystring_FIND_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/pystring"
            BUILD_BYPRODUCTS ${PYSTRING_LIBRARY}
            PATCH_COMMAND
                ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/share/cmake/projects/BuildPystring.cmake"
                "CMakeLists.txt"
            CMAKE_ARGS ${PYSTRING_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(pystring::pystring pystring_install)
        message(STATUS "Installing Pystring: ${PYSTRING_LIBRARY} (version ${PYSTRING_VERSION})")
    endif()
endif()

###############################################################################
### Configure target ###

if(_PYSTRING_TARGET_CREATE)
    set_target_properties(pystring::pystring PROPERTIES
        IMPORTED_LOCATION ${PYSTRING_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${PYSTRING_INCLUDE_DIR}
    )

    mark_as_advanced(PYSTRING_INCLUDE_DIR PYSTRING_LIBRARY PYSTRING_VERSION)
endif()
