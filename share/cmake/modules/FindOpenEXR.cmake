# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install OpenEXR.
#
# Variables defined by this module:
#   OpenEXR_FOUND - If FALSE, do not try to link to OpenEXR
#   OpenEXR_LIBRARY - OpenEXR library to link to
#   OpenEXR_INCLUDE_DIR - Where to find OpenEXR headers
#   OpenEXR_VERSION - The version of the library
#
# Imported targets defined by this module, if found:
#   OpenEXR::Iex
#   OpenEXR::IexConfig
#   OpenEXR::IlmThread
#   OpenEXR::IlmThreadConfig
#   OpenEXR::OpenEXR
#   OpenEXR::OpenEXRConfig
#   OpenEXR::OpenEXRCore
#   OpenEXR::OpenEXRUtil
#
# By default, the dynamic libraries of OpenEXR will be found. To find the
# static ones instead, you must set the OpenEXR_STATIC_LIBRARY variable to
# TRUE before calling find_package(OpenEXR ...).
#
# If OpenEXR is not installed in a standard path, you can use the
# OpenEXR_ROOT variable to tell CMake where to find it. If it is not found
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, OpenEXR will be
# downloaded, statically-built, and used to generate
# libOpenColorIOimageioapphelpers.
#

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_OpenEXR_REQUIRED_VARS OpenEXR_LIBRARY)
    set(_OpenEXR_LIB_VER "${OpenEXR_FIND_VERSION_MAJOR}_${OpenEXR_FIND_VERSION_MINOR}")

    if(NOT DEFINED OpenEXR_ROOT)
        # Search for OpenEXRConfig.cmake
        find_package(OpenEXR ${OpenEXR_FIND_VERSION} CONFIG QUIET)
    endif()

    if(OpenEXR_FOUND)
        get_target_property(OpenEXR_LIBRARY OpenEXR::OpenEXR LOCATION)
        get_target_property(OpenEXR_INCLUDE_DIR OpenEXR::OpenEXR INTERFACE_INCLUDE_DIRECTORIES)
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(OpenEXR_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(OpenEXR
        REQUIRED_VARS
            ${_OpenEXR_REQUIRED_VARS}
        VERSION_VAR
            OpenEXR_VERSION
    )
endif()

###############################################################################
### Create target

if (NOT TARGET OpenEXR::OpenEXR)
    set(_OpenEXR_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

macro(set_target_location target_name)
    if(NOT DEFINED "${target_name}_LIBRARY")
        set("${target_name}_LIBRARY" "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${target_name}-${_OpenEXR_LIB_VER}${_OpenEXR_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endif()
endmacro()

if(NOT OpenEXR_FOUND AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Required dependency
    # OCIO custom module to find ZLIB. (Findzlib)
    find_package(zlib)
    if(NOT ZLIB_FOUND)
        message(STATUS "ZLib is required to build OpenEXR.")
        return()
    endif()

    find_package(Threads)
    if(NOT Threads_FOUND)
        message(STATUS "Threads is required to build OpenEXR.")
        return()
    endif()

    # Set find_package standard args
    set(OpenEXR_FOUND TRUE)
    if(_OpenEXR_ExternalProject_VERSION)
        set(OpenEXR_VERSION ${_OpenEXR_ExternalProject_VERSION})
    else()
        set(OpenEXR_VERSION ${OpenEXR_FIND_VERSION})
    endif()
    set(OpenEXR_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    # Set the expected library name
    if(BUILD_TYPE_DEBUG)
        set(_OpenEXR_LIB_SUFFIX "_d")
    endif()

    include(VersionUtils)
    split_version_string(${OpenEXR_VERSION} _OpenEXR_ExternalProject)

    set(_OpenEXR_LIB_VER "${_OpenEXR_ExternalProject_VERSION_MAJOR}_${_OpenEXR_ExternalProject_VERSION_MINOR}")

    set_target_location(Iex)
    set_target_location(IlmThread)
    set_target_location(OpenEXR)
    set_target_location(OpenEXRCore)
    set_target_location(OpenEXRUtil)

    if(_OpenEXR_TARGET_CREATE)
        if(MSVC)
            set(OpenEXR_CXX_FLAGS "${OpenEXR_CXX_FLAGS} /EHsc")
        endif()

        string(STRIP "${OpenEXR_CXX_FLAGS}" OpenEXR_CXX_FLAGS)

        set(OpenEXR_CMAKE_ARGS
            ${OpenEXR_CMAKE_ARGS}
            -DCMAKE_CXX_VISIBILITY_PRESET=${CMAKE_CXX_VISIBILITY_PRESET}
            -DCMAKE_VISIBILITY_INLINES_HIDDEN=${CMAKE_VISIBILITY_INLINES_HIDDEN}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${OpenEXR_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
            -DCMAKE_INSTALL_DATADIR=${CMAKE_INSTALL_DATADIR}
            -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DBUILD_SHARED_LIBS=OFF
            -DBUILD_TESTING=OFF
            -DOPENEXR_INSTALL_EXAMPLES=OFF
            -DOPENEXR_INSTALL_TOOLS=OFF
            # Try to use in-source built Imath first, if available.
            -DCMAKE_PREFIX_PATH=${_EXT_DIST_ROOT}
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(OpenEXR_CMAKE_ARGS
                ${OpenEXR_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            string(REPLACE ";" "$<SEMICOLON>" ESCAPED_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

            set(OpenEXR_CMAKE_ARGS
                ${OpenEXR_CMAKE_ARGS}
                -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_OSX_ARCHITECTURES=${ESCAPED_CMAKE_OSX_ARCHITECTURES}
            )
        endif()

        if (ANDROID)
            set(OpenEXR_CMAKE_ARGS
                ${OpenEXR_CMAKE_ARGS}
                -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_STL=${ANDROID_STL})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${OpenEXR_INCLUDE_DIR})

        ExternalProject_Add(openexr_install
            GIT_REPOSITORY "https://github.com/AcademySoftwareFoundation/openexr.git"
            GIT_TAG "v${OpenEXR_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/openexr"
            BUILD_BYPRODUCTS
                ${Iex_LIBRARY}
                ${IlmThread_LIBRARY}
                ${OpenEXR_LIBRARY}
                ${OpenEXRCore_LIBRARY}
                ${OpenEXRUtil_LIBRARY}
            CMAKE_ARGS ${OpenEXR_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
            BUILD_COMMAND ""
            INSTALL_COMMAND
                ${CMAKE_COMMAND} --build .
                                 --config ${CMAKE_BUILD_TYPE}
                                 --target install
                                 --parallel
        )

        # Additional targets. ALIAS to UNKNOWN imported target is only possible
        # from CMake 3.15, so we explicitly define targets as STATIC here.
        add_library(OpenEXR::Iex STATIC IMPORTED GLOBAL)
        add_library(OpenEXR::IexConfig INTERFACE IMPORTED GLOBAL)
        add_library(OpenEXR::IlmThread STATIC IMPORTED GLOBAL)
        add_library(OpenEXR::IlmThreadConfig INTERFACE IMPORTED GLOBAL)
        add_library(OpenEXR::OpenEXR STATIC IMPORTED GLOBAL)
        add_library(OpenEXR::OpenEXRConfig INTERFACE IMPORTED GLOBAL)
        add_library(OpenEXR::OpenEXRCore STATIC IMPORTED GLOBAL)
        add_library(OpenEXR::OpenEXRUtil STATIC IMPORTED GLOBAL)

        add_dependencies(OpenEXR::OpenEXR openexr_install)

        message(STATUS "Installing OpenEXR: ${OpenEXR_LIBRARY} (version \"${OpenEXR_VERSION}\")")
    endif()
endif()

###############################################################################
### Configure target ###

if(_OpenEXR_TARGET_CREATE)
    file(MAKE_DIRECTORY ${OpenEXR_INCLUDE_DIR}/OpenEXR)

    set_target_properties(OpenEXR::Iex PROPERTIES
        IMPORTED_LOCATION ${Iex_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES "${OpenEXR_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "OpenEXR::IlmThreadConfig;OpenEXR::IlmThreadConfig"
    )
    set_target_properties(OpenEXR::IexConfig PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OpenEXR_INCLUDE_DIR};${OpenEXR_INCLUDE_DIR}/OpenEXR"
    )
    set_target_properties(OpenEXR::IlmThread PROPERTIES
        IMPORTED_LOCATION ${IlmThread_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES "${OpenEXR_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "OpenEXR::IlmThreadConfig;OpenEXR::IlmThreadConfig;OpenEXR::Iex;Threads::Threads"
    )
    set_target_properties(OpenEXR::IlmThreadConfig PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OpenEXR_INCLUDE_DIR};${OpenEXR_INCLUDE_DIR}/OpenEXR"
    )
    set_target_properties(OpenEXR::OpenEXR PROPERTIES
        IMPORTED_LOCATION ${OpenEXR_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES "${OpenEXR_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "OpenEXR::IlmThreadConfig;Imath::Imath;OpenEXR::IlmThreadConfig;OpenEXR::Iex;OpenEXR::IlmThread;ZLIB::ZLIB"
    )
    set_target_properties(OpenEXR::OpenEXRConfig PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${OpenEXR_INCLUDE_DIR};${OpenEXR_INCLUDE_DIR}/OpenEXR"
    )
    set_target_properties(OpenEXR::OpenEXRCore PROPERTIES
        IMPORTED_LOCATION ${OpenEXRCore_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES "${OpenEXR_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "OpenEXR::IlmThreadConfig;ZLIB::ZLIB;\$<LINK_ONLY:Imath::ImathConfig>"
    )
    set_target_properties(OpenEXR::OpenEXRUtil PROPERTIES
        IMPORTED_LOCATION ${OpenEXRUtil_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES "${OpenEXR_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "OpenEXR::IlmThreadConfig;OpenEXR::OpenEXR"
    )

    mark_as_advanced(OpenEXR_INCLUDE_DIR OpenEXR_LIBRARY OpenEXR_VERSION)
endif()
