# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install ilmbase
#
# Variables defined by this module:
#   Half_FOUND - If FALSE, do not try to link to ilmbase
#   Half_LIBRARY - Half library to link to
#   Half_INCLUDE_DIR - Where to find half.h
#   Half_VERSION - The version of the library
#
# Targets defined by this module:
#   IlmBase::Half - IMPORTED target, if found
#
# By default, the dynamic libraries of ilmbase will be found. To find the 
# static ones instead, you must set the Half_STATIC_LIBRARY variable to 
# TRUE before calling find_package(Half ...).
#
# If IlmBase is not installed in a standard path, you can use the 
# Half_ROOT variable to tell CMake where to find it. If it is not found 
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, IlmBase will be 
# downloaded, built, and statically-linked into libOpenColorIO at build time.
#

# IlmBase components may have the version in their name
set(_Half_LIB_VER "${Half_FIND_VERSION_MAJOR}_${Half_FIND_VERSION_MINOR}")

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_Half_REQUIRED_VARS Half_LIBRARY)

    if(NOT DEFINED Half_ROOT)
        # Search for IlmBaseConfig.cmake
        find_package(IlmBase ${Half_FIND_VERSION} CONFIG QUIET)
    endif()

    if(Half_FOUND)
        get_target_property(Half_LIBRARY IlmBase::Half LOCATION)
    else()
        list(APPEND _Half_REQUIRED_VARS Half_INCLUDE_DIR)

        # Search for IlmBase.pc
        find_package(PkgConfig QUIET)
        pkg_check_modules(PC_IlmBase QUIET "IlmBase>=${Half_FIND_VERSION}")

        # Find include directory
        find_path(Half_INCLUDE_DIR
            NAMES
                OpenEXR/half.h
            HINTS
                ${Half_ROOT}
                ${PC_Half_INCLUDE_DIRS}
            PATH_SUFFIXES
                include
        )

        # Lib names to search for
        set(_Half_LIB_NAMES "Half-${_Half_LIB_VER}" Half)
        if(BUILD_TYPE_DEBUG)
            # Prefer Debug lib names
            list(INSERT _Half_LIB_NAMES 0 "Half-${_Half_LIB_VER}_d")
        endif()

        if(Half_STATIC_LIBRARY)
            # Prefer static lib names
            set(_Half_STATIC_LIB_NAMES 
                "${CMAKE_STATIC_LIBRARY_PREFIX}Half-${_Half_LIB_VER}${CMAKE_STATIC_LIBRARY_SUFFIX}"
                "${CMAKE_STATIC_LIBRARY_PREFIX}Half${CMAKE_STATIC_LIBRARY_SUFFIX}"
            )
            if(BUILD_TYPE_DEBUG)
                # Prefer static Debug lib names
                list(INSERT _Half_STATIC_LIB_NAMES 0
                    "${CMAKE_STATIC_LIBRARY_PREFIX}Half-${_Half_LIB_VER}_d${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endif()
        endif()

        # Find library
        find_library(Half_LIBRARY
            NAMES
                ${_Half_STATIC_LIB_NAMES} 
                ${_Half_LIB_NAMES}
            HINTS
                ${Half_ROOT}
                ${PC_Half_LIBRARY_DIRS}
            PATH_SUFFIXES
                lib64 lib 
        )

        # Get version from config header file
        if(Half_INCLUDE_DIR)
            if(EXISTS "${Half_INCLUDE_DIR}/OpenEXR/IlmBaseConfig.h")
                set(_Half_CONFIG "${Half_INCLUDE_DIR}/OpenEXR/IlmBaseConfig.h")
            elseif(EXISTS "${Half_INCLUDE_DIR}/OpenEXR/OpenEXRConfig.h")
                set(_Half_CONFIG "${Half_INCLUDE_DIR}/OpenEXR/OpenEXRConfig.h")
            endif()
        endif()

        if(_Half_CONFIG)
            file(STRINGS "${_Half_CONFIG}" _Half_VER_SEARCH 
                REGEX "^[ \t]*#define[ \t]+(OPENEXR|ILMBASE)_VERSION_STRING[ \t]+\"[.0-9]+\".*$")
            if(_Half_VER_SEARCH)
                string(REGEX REPLACE ".*#define[ \t]+(OPENEXR|ILMBASE)_VERSION_STRING[ \t]+\"([.0-9]+)\".*" 
                    "\\2" Half_VERSION "${_Half_VER_SEARCH}")
            endif()
        elseif(PC_Half_FOUND)
            set(Half_VERSION "${PC_Half_VERSION}")
        endif()
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(Half_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Half
        REQUIRED_VARS 
            ${_Half_REQUIRED_VARS}
        VERSION_VAR
            Half_VERSION
    )
endif()

###############################################################################
### Create target

if (NOT TARGET IlmBase::Half)
    add_library(IlmBase::Half UNKNOWN IMPORTED GLOBAL)
    set(_Half_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT Half_FOUND)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(Half_FOUND TRUE)
    set(Half_VERSION ${Half_FIND_VERSION})
    set(Half_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    # Set the expected library name. "_d" is appended to Debug Windows builds 
    # <= OpenEXR 2.3.0. In newer versions, it is appended to Debug libs on
    # all platforms.
    if(BUILD_TYPE_DEBUG AND (WIN32 OR Half_VERSION VERSION_GREATER "2.3.0"))
        set(_Half_LIB_SUFFIX "_d")
    endif()

    set(Half_LIBRARY
        "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}Half-${_Half_LIB_VER}${_Half_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_Half_TARGET_CREATE)
        if(MSVC)
            set(Half_CXX_FLAGS "${Half_CXX_FLAGS} /EHsc")
        endif()

        string(STRIP "${Half_CXX_FLAGS}" Half_CXX_FLAGS)
 
        set(Half_CMAKE_ARGS
            ${Half_CMAKE_ARGS}
            -DCMAKE_CXX_VISIBILITY_PRESET=${CMAKE_CXX_VISIBILITY_PRESET}
            -DCMAKE_VISIBILITY_INLINES_HIDDEN=${CMAKE_VISIBILITY_INLINES_HIDDEN}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${Half_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DBUILD_SHARED_LIBS=OFF
            -DBUILD_TESTING=OFF
            -DOPENEXR_VIEWERS_ENABLE=OFF
            -DPYILMBASE_ENABLE=OFF
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(Half_CMAKE_ARGS
                ${Half_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            set(Half_CMAKE_ARGS
                ${Half_CMAKE_ARGS} -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${Half_INCLUDE_DIR})

        ExternalProject_Add(ilmbase_install
            GIT_REPOSITORY "https://github.com/AcademySoftwareFoundation/openexr.git"
            GIT_TAG "v${Half_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/openexr"
            BUILD_BYPRODUCTS ${Half_LIBRARY}
            CMAKE_ARGS ${Half_CMAKE_ARGS}
            PATCH_COMMAND
                ${CMAKE_COMMAND} -P "${CMAKE_SOURCE_DIR}/share/cmake/scripts/PatchOpenEXR.cmake"
            BUILD_COMMAND
                ${CMAKE_COMMAND} --build .
                                 --config ${CMAKE_BUILD_TYPE}
                                 --target Half
            INSTALL_COMMAND
                ${CMAKE_COMMAND} -DCMAKE_INSTALL_CONFIG_NAME=${CMAKE_BUILD_TYPE}
                                 -P "IlmBase/Half/cmake_install.cmake"
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(IlmBase::Half ilmbase_install)
        message(STATUS "Installing Half (IlmBase): ${Half_LIBRARY} (version \"${Half_VERSION}\")")
    endif()
endif()

###############################################################################
### Configure target ###

if(_Half_TARGET_CREATE)
    set_target_properties(IlmBase::Half PROPERTIES
        IMPORTED_LOCATION ${Half_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${Half_INCLUDE_DIR}
    )

    mark_as_advanced(Half_INCLUDE_DIR Half_LIBRARY Half_VERSION)
endif()
