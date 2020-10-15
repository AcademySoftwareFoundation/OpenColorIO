# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install pystring
#
# Variables defined by this module:
#   pystring_FOUND - If FALSE, do not try to link to pystring
#   pystring_LIBRARY - Where to find pystring
#   pystring_INCLUDE_DIR - Where to find pystring.h
#
# Targets defined by this module:
#   pystring::pystring - IMPORTED target, if found
#
# By default, the dynamic libraries of pystring will be found. To find the 
# static ones instead, you must set the pystring_STATIC_LIBRARY variable to 
# TRUE before calling find_package(pystring ...).
#
# If pystring is not installed in a standard path, you can use the 
# pystring_ROOT variable to tell CMake where to find it. If it is not found 
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, pystring will be 
# downloaded, built, and statically-linked into libOpenColorIO at build time.
#

if(NOT TARGET pystring::pystring)
    add_library(pystring::pystring UNKNOWN IMPORTED GLOBAL)
    set(_pystring_TARGET_CREATE TRUE)
endif()

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Find include directory
    find_path(pystring_INCLUDE_DIR
        NAMES
            pystring/pystring.h
        HINTS
            ${pystring_ROOT}
        PATH_SUFFIXES
            include
            pystring/include
    )

    # Attempt to find static library first if this is set
    if(pystring_STATIC_LIBRARY)
        set(_pystring_STATIC 
            "${CMAKE_STATIC_LIBRARY_PREFIX}pystring${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endif()

    # Find library
    find_library(pystring_LIBRARY
        NAMES
            ${_pystring_STATIC} pystring
        HINTS
            ${_pystring_SEARCH_DIRS}
        PATH_SUFFIXES
            lib64 lib 
    )

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(pystring_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(pystring
        REQUIRED_VARS 
            pystring_INCLUDE_DIR 
            pystring_LIBRARY
    )
    set(pystring_FOUND ${pystring_FOUND})
endif()

###############################################################################
### Install package from source ###

if(NOT pystring_FOUND)
    include(ExternalProject)

    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})
    endif()

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(pystring_FOUND TRUE)
    set(pystring_VERSION ${pystring_FIND_VERSION})
    set(pystring_INCLUDE_DIR "${_EXT_DIST_ROOT}/include")
    set(pystring_LIBRARY 
        "${_EXT_DIST_ROOT}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}pystring${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_pystring_TARGET_CREATE)
        if(UNIX)
            set(pystring_CXX_FLAGS "${pystring_CXX_FLAGS} -fvisibility=hidden -fPIC")
            if(OCIO_INLINES_HIDDEN)
                set(pystring_CXX_FLAGS "${pystring_CXX_FLAGS} -fvisibility-inlines-hidden")
            endif()
        endif()

        if(MSVC)
            set(pystring_CXX_FLAGS "${pystring_CXX_FLAGS} /EHsc")
        endif()

        string(STRIP "${pystring_CXX_FLAGS}" pystring_CXX_FLAGS)

        set(pystring_CMAKE_ARGS
            ${pystring_CMAKE_ARGS}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${pystring_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
        )
        if(CMAKE_TOOLCHAIN_FILE)
            set(pystring_CMAKE_ARGS
                ${pystring_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(NOT BUILD_SHARED_LIBS)
            #TODO: Find a way to merge in the static libs when built with internal pystring
            message(WARNING
                "Building STATIC libOpenColorIO using the in-built pystring. "
                "pystring symbols are NOT included in the output binary!"
            )
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${pystring_INCLUDE_DIR})

        ExternalProject_Add(pystring_install
            GIT_REPOSITORY "https://github.com/imageworks/pystring.git"
            GIT_TAG "v${pystring_FIND_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/pystring"
            BUILD_BYPRODUCTS ${pystring_LIBRARY}
            PATCH_COMMAND
                ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/share/cmake/projects/Buildpystring.cmake"
                "CMakeLists.txt"
            CMAKE_ARGS ${pystring_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(pystring::pystring pystring_install)
        message(STATUS "Installing pystring: ${pystring_LIBRARY} (version \"${pystring_VERSION}\")")
    endif()
endif()

###############################################################################
### Configure target ###

if(_pystring_TARGET_CREATE)
    set_target_properties(pystring::pystring PROPERTIES
        IMPORTED_LOCATION ${pystring_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${pystring_INCLUDE_DIR}
    )

    mark_as_advanced(pystring_INCLUDE_DIR pystring_LIBRARY pystring_VERSION)
endif()
