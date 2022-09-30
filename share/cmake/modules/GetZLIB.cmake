# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install minizip-ng
#
# Variables defined by this module:
#   ZLIB_FOUND          - If FALSE, do not try to link to minizip-ng
#   ZLIB_LIBRARIES      - ZLIB library to link to
#   ZLIB_INCLUDE_DIRS   - Where to find zlib.h and other headers
#   ZLIB_VERSION        - The version of the library
#
# Targets defined by this module:
#   ZLIB::ZLIB - IMPORTED target, if found
#
# This module is named GetZLIB because it is not used with find_package().
# It must be included using include(). 
#
# The reason is that CMake provide a FindZLIB already and the current file is
# using it.
#
###############################################################################
### Try to find package ###

# Assign the rigtt name for ZLIB depending on the OS.
if(UNIX)
    set(ZLIB_NAME libz)
else()
    set(ZLIB_NAME zlib)
endif()

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_ZLIB_REQUIRED_VARS ZLIB_LIBRARIES)

    if(NOT DEFINED ZLIB_ROOT)
        # findZLIB provided by CMAKE.
        find_package(ZLIB ${ZLIB_FIND_VERSION})
    endif()
endif()

###############################################################################
### Create target

if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB UNKNOWN IMPORTED GLOBAL)
    set(_ZLIB_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###
if(NOT ZLIB_FOUND AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(ZLIB_FOUND TRUE)
    set(ZLIB_VERSION ${ZLIB_FIND_VERSION})
    set(ZLIB_INCLUDE_DIRS "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    set(ZLIB_LIBRARIES
        "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${_ZLIB_LIB_PREFIX}${ZLIB_NAME}${_ZLIB_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_ZLIB_TARGET_CREATE)
        set(ZLIB_CMAKE_ARGS
            ${ZLIB_CMAKE_ARGS}
            -DCMAKE_CXX_VISIBILITY_PRESET=${CMAKE_CXX_VISIBILITY_PRESET}
            -DCMAKE_VISIBILITY_INLINES_HIDDEN=${CMAKE_VISIBILITY_INLINES_HIDDEN}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
            -DCMAKE_INSTALL_DATADIR=${CMAKE_INSTALL_DATADIR}
            -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(ZLIB_CMAKE_ARGS
                ${ZLIB_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            string(REPLACE ";" "$<SEMICOLON>" ESCAPED_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

            set(ZLIB_CMAKE_ARGS
                ${ZLIB_CMAKE_ARGS}
                -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_OSX_ARCHITECTURES=${ESCAPED_CMAKE_OSX_ARCHITECTURES}
            )
        endif()

        if (ANDROID)
            set(ZLIB_CMAKE_ARGS
                ${ZLIB_CMAKE_ARGS}
                -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_STL=${ANDROID_STL})
        endif()
    endif()

    # Hack to let imported target be built from ExternalProject_Add
    file(MAKE_DIRECTORY ${ZLIB_INCLUDE_DIRS})
    
    ExternalProject_Add(ZLIB_install
        GIT_REPOSITORY "https://github.com/madler/zlib.git"
        GIT_TAG "v${ZLIB_VERSION}"
        GIT_CONFIG advice.detachedHead=false
        GIT_SHALLOW TRUE
        PREFIX "${_EXT_BUILD_ROOT}/${ZLIB_NAME}"
        BUILD_BYPRODUCTS ${ZLIB_LIBRARIES}
        CMAKE_ARGS ${ZLIB_CMAKE_ARGS}
        EXCLUDE_FROM_ALL TRUE
        BUILD_COMMAND ""
        INSTALL_COMMAND
            ${CMAKE_COMMAND} --build .
                             --config ${CMAKE_BUILD_TYPE}
                             --target install
                             --parallel
    )

    add_dependencies(ZLIB::ZLIB ZLIB_install)
    message(STATUS "Installing ZLIB: ${ZLIB_LIBRARIES} (version \"${ZLIB_VERSION}\")")
endif()

###############################################################################
### Configure target ###

if(_ZLIB_TARGET_CREATE)
    set_target_properties(ZLIB::ZLIB PROPERTIES
        IMPORTED_LOCATION ${ZLIB_LIBRARIES}
        INTERFACE_INCLUDE_DIRECTORIES ${ZLIB_INCLUDE_DIRS}
    )

    mark_as_advanced(ZLIB_INCLUDE_DIRS ZLIB_LIBRARIES ZLIB_VERSION)
endif()