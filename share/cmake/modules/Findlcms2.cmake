# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install lcms2
#
# Variables defined by this module:
#   lcms2_FOUND - If FALSE, do not try to link to lcms
#   lcms2_LIBRARY - Where to find lcms
#   lcms2_INCLUDE_DIR - Where to find lcms2.h
#   lcms2_VERSION - The version of the library
#
# Targets defined by this module:
#   lcms2::lcms2 - IMPORTED target, if found
#
# By default, the dynamic libraries of lcms2 will be found. To find the static 
# ones instead, you must set the lcms2_STATIC_LIBRARY variable to TRUE 
# before calling find_package(lcms2 ...).
#
# If lcms2 is not installed in a standard path, you can use the lcms2_ROOT 
# variable to tell CMake where to find it. If it is not found and 
# OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, lcms2 will be 
# downloaded, built, and statically-linked into libOpenColorIO at build time.
#

if(NOT TARGET lcms2::lcms2)
    add_library(lcms2::lcms2 UNKNOWN IMPORTED GLOBAL)
    set(_lcms2_TARGET_CREATE TRUE)
endif()

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Try to use pkg-config to get the version
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_lcms2 QUIET "lcms2>=${lcms2_FIND_VERSION}")

    # Find include directory
    find_path(lcms2_INCLUDE_DIR
        NAMES
            lcms2.h
        HINTS
            ${lcms2_ROOT}
            ${PC_lcms2_INCLUDE_DIRS}
        PATH_SUFFIXES
            include
            lcms2/include
            lcms2/include/lcms2
            include/lcms2
    )

    # Attempt to find static library first if this is set
    if(lcms2_STATIC_LIBRARY)
        set(_lcms2_STATIC "${CMAKE_STATIC_LIBRARY_PREFIX}lcms2${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endif()

    # Find library
    find_library(lcms2_LIBRARY
        NAMES
            ${_lcms2_STATIC} lcms2 liblcms2
        HINTS
            ${lcms2_ROOT}
            ${PC_lcms2_LIBRARY_DIRS}
        PATH_SUFFIXES
            lcms2/lib
            lib64
            lib
    )

    # Get version from config or header file
    if(lcms2_INCLUDE_DIR AND EXISTS "${lcms2_INCLUDE_DIR}/lcms2.h")
        file(STRINGS "${lcms2_INCLUDE_DIR}/lcms2.h" _lcms2_VER_SEARCH 
            REGEX "^[ \t]*//[ \t]+Version[ \t]+[.0-9]+.*$")
        if(_lcms2_VER_SEARCH)
            string(REGEX REPLACE ".*//[ \t]+Version[ \t]+([.0-9]+).*" 
                "\\1" lcms2_VERSION "${_lcms2_VER_SEARCH}")
        endif()
    elseif(PC_lcms2_FOUND)
        set(lcms2_VERSION "${PC_lcms2_VERSION}")
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(lcms2_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(lcms2
        REQUIRED_VARS 
            lcms2_INCLUDE_DIR 
            lcms2_LIBRARY 
        VERSION_VAR
            lcms2_VERSION
    )
endif()

###############################################################################
### Install package from source ###

if(NOT lcms2_FOUND AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(lcms2_FOUND TRUE)
    set(lcms2_VERSION ${lcms2_FIND_VERSION})
    set(lcms2_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}/lcms2")
    set(lcms2_LIBRARY 
        "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}lcms2${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_lcms2_TARGET_CREATE)
        if(UNIX)
            set(lcms2_C_FLAGS "${lcms2_C_FLAGS} -fvisibility=hidden -fPIC")
        endif()

        if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
            set(lcms2_C_FLAGS "${lcms2_C_FLAGS} -Wno-aggressive-loop-optimizations")
        endif()

        string(STRIP "${lcms2_C_FLAGS}" lcms2_C_FLAGS)

        # NOTE: Depending of the compiler version lcm2 2.2 does not compile with C++17 so revert 
        # to C++11 because the library is only used by a cmd line tool.

        set(lcms2_CXX_STANDARD ${CMAKE_CXX_STANDARD})
        if(${CMAKE_CXX_STANDARD} GREATER_EQUAL 17)
            set(lcms2_CXX_STANDARD 11)
        endif()

        set(lcms2_CMAKE_ARGS
            ${lcms2_CMAKE_ARGS}
            -DCMAKE_C_FLAGS=${lcms2_C_FLAGS}
            -DCMAKE_CXX_STANDARD=${lcms2_CXX_STANDARD}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
            -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DBUILD_SHARED_LIBS=OFF
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(lcms2_CMAKE_ARGS
                ${lcms2_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            string(REPLACE ";" "$<SEMICOLON>" ESCAPED_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

            set(lcms2_CMAKE_ARGS
                ${lcms2_CMAKE_ARGS}
                -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_OSX_ARCHITECTURES=${ESCAPED_CMAKE_OSX_ARCHITECTURES}
            )
        endif()

        if (ANDROID)
            set(lcms2_CMAKE_ARGS
                ${lcms2_CMAKE_ARGS}
                -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_STL=${ANDROID_STL})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${lcms2_INCLUDE_DIR})

        ExternalProject_Add(lcms2_install
            GIT_REPOSITORY "https://github.com/mm2/Little-CMS.git"
            GIT_TAG "lcms${lcms2_VERSION}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/Little-CMS"
            BUILD_BYPRODUCTS ${lcms2_LIBRARY}
            CMAKE_ARGS ${lcms2_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
            PATCH_COMMAND
                ${CMAKE_COMMAND} -E copy
                "${CMAKE_SOURCE_DIR}/share/cmake/projects/Buildlcms2.cmake"
                "CMakeLists.txt"
            BUILD_COMMAND ""
            INSTALL_COMMAND
                ${CMAKE_COMMAND} --build .
                                 --config ${CMAKE_BUILD_TYPE}
                                 --target install
                                 --parallel
        )

        add_dependencies(lcms2::lcms2 lcms2_install)
        message(STATUS "Installing lcms2: ${lcms2_LIBRARY} (version \"${lcms2_VERSION}\")")
    endif()
endif()

###############################################################################
### Configure target ###

if(_lcms2_TARGET_CREATE)
    set_target_properties(lcms2::lcms2 PROPERTIES
        IMPORTED_LOCATION ${lcms2_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${lcms2_INCLUDE_DIR}
    )

    mark_as_advanced(lcms2_INCLUDE_DIR lcms2_LIBRARY lcms2_VERSION)
endif()
