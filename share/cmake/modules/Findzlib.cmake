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
if(WIN32)
    set(_ZLIB_LIB_NAME "zlib")
    set(_ZLIB_STATIC_LIB_NAME "zlibstatic")
else()
    set(_ZLIB_LIB_NAME "z")
    set(_ZLIB_STATIC_LIB_NAME "z")
endif()

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_ZLIB_REQUIRED_VARS ZLIB_LIBRARIES)

    if(NOT DEFINED ZLIB_ROOT)
        # Save old value of CMAKE_MODULE_PATH 
        set(_ZLIB__CMAKE_MODULE_PATH_OLD_ ${CMAKE_MODULE_PATH})
        # Force find_package to use CMAKE module and not custom modules.
        set(CMAKE_MODULE_PATH "${CMAKE_ROOT}/Modules") 

        # Use CMake FindZLIB module
        find_package(ZLIB ${zlib_FIND_VERSION})

        # Restore CMAKE_MODULE_PATH
        set(CMAKE_MODULE_PATH ${_ZLIB__CMAKE_MODULE_PATH_OLD_})
    endif()
endif()

###############################################################################
### Create target

if(NOT TARGET ZLIB::ZLIB)
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
    if(_zlib_ExternalProject_VERSION)
        set(ZLIB_VERSION ${_zlib_ExternalProject_VERSION})
    else()
        set(ZLIB_VERSION ${zlib_FIND_VERSION})
    endif()
    set(ZLIB_INCLUDE_DIRS "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    # Windows need the "d" suffix at the end.
    if(WIN32 AND BUILD_TYPE_DEBUG)
        set(_ZLIB_LIB_SUFFIX "d")
    endif()

    # ZLIB doesn't use CMAKE_INSTALL_LIBDIR
    # It is hardcoded to ${CMAKE_INSTALL_PREFIX}/lib
    # https://github.com/madler/zlib/blob/master/CMakeLists.txt#L9
    set(_ZLIB_INSTALL_LIBDIR "lib")
    set(ZLIB_LIBRARIES
        "${_EXT_DIST_ROOT}/${_ZLIB_INSTALL_LIBDIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${_ZLIB_STATIC_LIB_NAME}${_ZLIB_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

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
        PREFIX "${_EXT_BUILD_ROOT}/zlib"
        BUILD_BYPRODUCTS 
            ${ZLIB_LIBRARIES}
        CMAKE_ARGS ${ZLIB_CMAKE_ARGS}
        EXCLUDE_FROM_ALL TRUE
        BUILD_COMMAND ""
        INSTALL_COMMAND
            ${CMAKE_COMMAND} --build .
                             --config ${CMAKE_BUILD_TYPE}
                             --target install
                             --parallel
    )

    ExternalProject_Add_Step(
        ZLIB_install zlib_remove_dll
        COMMENT "Remove zlib.lib and zlib.dll, leaves only zlibstatic.lib"
        DEPENDEES install
        COMMAND ${CMAKE_COMMAND} -E remove -f ${_EXT_DIST_ROOT}/${_ZLIB_INSTALL_LIBDIR}/zlib.lib ${_EXT_DIST_ROOT}/bin/zlib.dll
    )

    add_library(ZLIB::ZLIB STATIC IMPORTED GLOBAL)
    add_dependencies(ZLIB::ZLIB ZLIB_install)
    set_property(TARGET ZLIB::ZLIB PROPERTY
        IMPORTED_LOCATION "${ZLIB_LIBRARIES}"
    )
    target_include_directories(ZLIB::ZLIB INTERFACE "${CMAKE_INSTALL_BINDIR}/include")

    set(ZLIB_LIBRARY ${ZLIB_LIBRARIES})
    set(ZLIB_INCLUDE_DIR ${ZLIB_INCLUDE_DIRS})

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