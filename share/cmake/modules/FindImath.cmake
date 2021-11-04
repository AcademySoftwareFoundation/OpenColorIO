# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install Imath
#
# Variables defined by this module:
#   Imath_FOUND - If FALSE, do not try to link to ilmbase
#   Imath_LIBRARY - Imath library to link to
#   Imath_INCLUDE_DIR - Where to find ImathConfig.h
#   Imath_VERSION - The version of the library
#
# Targets defined by this module:
#   Imath::Imath - IMPORTED target, if found
#
# By default, the dynamic libraries of Imath will be found. To find the 
# static ones instead, you must set the Imath_STATIC_LIBRARY variable to 
# TRUE before calling find_package(Imath ...).
#
# If Imath is not installed in a standard path, you can use the 
# Imath_ROOT variable to tell CMake where to find it. If it is not found 
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, Imath will be 
# downloaded, built, and statically-linked into libOpenColorIO at build time.
#

# Imath components may have the version in their name
set(_Imath_LIB_VER "${Imath_FIND_VERSION_MAJOR}_${Imath_FIND_VERSION_MINOR}")

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_Imath_REQUIRED_VARS Imath_LIBRARY)

    if(NOT DEFINED Imath_ROOT)
        # Search for ImathConfig.cmake
        find_package(Imath ${Imath_FIND_VERSION} CONFIG QUIET)
    endif()

    if(Imath_FOUND)
        get_target_property(Imath_LIBRARY Imath::Imath LOCATION)
    else()
        list(APPEND _Imath_REQUIRED_VARS Imath_INCLUDE_DIR)

        # Search for Imath.pc
        find_package(PkgConfig QUIET)
        pkg_check_modules(PC_Imath QUIET "Imath>=${Imath_FIND_VERSION}")

        # Find include directory
        find_path(Imath_INCLUDE_DIR
            NAMES
                Imath/ImathConfig.h
            HINTS
                ${Imath_ROOT}
                ${PC_Imath_INCLUDE_DIRS}
            PATH_SUFFIXES
                include
        )

        # Lib names to search for
        set(_Imath_LIB_NAMES "Imath-${_Imath_LIB_VER}" Imath)
        if(BUILD_TYPE_DEBUG)
            # Prefer Debug lib names
            list(INSERT _Imath_LIB_NAMES 0 "Imath-${_Imath_LIB_VER}_d")
        endif()

        if(Imath_STATIC_LIBRARY)
            # Prefer static lib names
            set(_Imath_STATIC_LIB_NAMES 
                "${CMAKE_STATIC_LIBRARY_PREFIX}Imath-${_Imath_LIB_VER}${CMAKE_STATIC_LIBRARY_SUFFIX}"
                "${CMAKE_STATIC_LIBRARY_PREFIX}Imath${CMAKE_STATIC_LIBRARY_SUFFIX}"
            )
            if(BUILD_TYPE_DEBUG)
                # Prefer static Debug lib names
                list(INSERT _Imath_STATIC_LIB_NAMES 0
                    "${CMAKE_STATIC_LIBRARY_PREFIX}Imath-${_Imath_LIB_VER}_d${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endif()
        endif()

        # Find library
        find_library(Imath_LIBRARY
            NAMES
                ${_Imath_STATIC_LIB_NAMES} 
                ${_Imath_LIB_NAMES}
            HINTS
                ${Imath_ROOT}
                ${PC_Imath_LIBRARY_DIRS}
            PATH_SUFFIXES
                lib64 lib 
        )

        # Get version from config header file
        if(Imath_INCLUDE_DIR)
            if(EXISTS "${Imath_INCLUDE_DIR}/Imath/ImathConfig.h")
                set(_Imath_CONFIG "${Imath_INCLUDE_DIR}/Imath/ImathConfig.h")
            endif()
        endif()

        if(_Imath_CONFIG)
            file(STRINGS "${_Imath_CONFIG}" _Imath_VER_SEARCH 
                REGEX "^[ \t]*#define[ \t]+IMATH_VERSION_STRING[ \t]+\"[.0-9]+\".*$")
            if(_Imath_VER_SEARCH)
                string(REGEX REPLACE ".*#define[ \t]+IMATH_VERSION_STRING[ \t]+\"([.0-9]+)\".*" 
                    "\\1" Imath_VERSION "${_Imath_VER_SEARCH}")
            endif()
        elseif(PC_Imath_FOUND)
            set(Imath_VERSION "${PC_Imath_VERSION}")
        endif()
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(Imath_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Imath
        REQUIRED_VARS 
            ${_Imath_REQUIRED_VARS}
        VERSION_VAR
            Imath_VERSION
    )
endif()

###############################################################################
### Create target

if (NOT TARGET Imath::Imath)
    add_library(Imath::Imath UNKNOWN IMPORTED GLOBAL)
    set(_Imath_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT Imath_FOUND)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(Imath_FOUND TRUE)
    set(Imath_VERSION ${Imath_FIND_VERSION})
    set(Imath_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    # Set the expected library name. "_d" is appended to Debug Windows builds 
    # <= OpenEXR 2.3.0. In newer versions, it is appended to Debug libs on
    # all platforms.
    if(BUILD_TYPE_DEBUG)
        set(_Imath_LIB_SUFFIX "_d")
    endif()

    set(Imath_LIBRARY
        "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}Imath-${_Imath_LIB_VER}${_Imath_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_Imath_TARGET_CREATE)
        if(MSVC)
            set(Imath_CXX_FLAGS "${Imath_CXX_FLAGS} /EHsc")
        endif()

        string(STRIP "${Imath_CXX_FLAGS}" Imath_CXX_FLAGS)
 
        set(Imath_CMAKE_ARGS
            ${Imath_CMAKE_ARGS}
            -DCMAKE_CXX_VISIBILITY_PRESET=${CMAKE_CXX_VISIBILITY_PRESET}
            -DCMAKE_VISIBILITY_INLINES_HIDDEN=${CMAKE_VISIBILITY_INLINES_HIDDEN}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${Imath_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DBUILD_SHARED_LIBS=OFF
            -DBUILD_TESTING=OFF
            -DPYTHON=OFF
            -DIMATH_HALF_USE_LOOKUP_TABLE=OFF
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(Imath_CMAKE_ARGS
                ${Imath_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            set(Imath_CMAKE_ARGS
                ${Imath_CMAKE_ARGS} -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${Imath_INCLUDE_DIR})

        ExternalProject_Add(imath_install
            GIT_REPOSITORY "https://github.com/AcademySoftwareFoundation/Imath.git"
            GIT_TAG "v${Imath_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/Imath"
            BUILD_BYPRODUCTS ${Imath_LIBRARY}
            CMAKE_ARGS ${Imath_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(Imath::Imath imath_install)

        # Some Imath versions define a second target. 
        add_library(Imath::ImathConfig ALIAS Imath::Imath)

        message(STATUS "Installing Imath: ${Imath_LIBRARY} (version \"${Imath_VERSION}\")")
    endif()
endif()

###############################################################################
### Configure target ###

if(_Imath_TARGET_CREATE)
    set_target_properties(Imath::Imath PROPERTIES
        IMPORTED_LOCATION ${Imath_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${Imath_INCLUDE_DIR}
    )

    mark_as_advanced(Imath_INCLUDE_DIR Imath_LIBRARY Imath_VERSION)
endif()
