# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install ZLIB
# 
# **********************************************************************
# Note that this is a wrapper around the CMake ZLIB find module.
# This find module DOES NOT output any variables with lowercase "zlib".
# 
# Treat this module as if it was FindZLIB.cmake. 
# **********************************************************************
#
# Variables defined by this module:
#   ZLIB_FOUND          - If FALSE, do not try to link to zlib
#   ZLIB_LIBRARIES      - ZLIB library to link to
#   ZLIB_INCLUDE_DIRS   - Where to find zlib.h and other headers
#   ZLIB_VERSION        - The version of the library
#
# Targets defined by this module:
#   ZLIB::ZLIB - IMPORTED target, if found
#
###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Update ZLIB_ROOT if zlib_ROOT was set.
    if (zlib_ROOT)
        set(ZLIB_ROOT "${zlib_ROOT}")
    endif()

    # Update ZLIB_LIBRARY if zlib_LIBRARY was set. 
    if (zlib_LIBRARY)
        set(ZLIB_LIBRARY "${zlib_LIBRARY}")
    endif()

    # Update ZLIB_INCLUDE_DIR if zlib_INCLUDE_DIR was set. 
    if (zlib_INCLUDE_DIR)
        set(ZLIB_INCLUDE_DIR "${zlib_INCLUDE_DIR}")
    endif()

    # ZLIB_USE_STATIC_LIBS is supported only from CMake 3.24+.
    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0") 
        if (ZLIB_STATIC_LIBRARY)
            set(ZLIB_USE_STATIC_LIBS "${ZLIB_STATIC_LIBRARY}")
        elseif(zlib_STATIC_LIBRARY)
            set(ZLIB_USE_STATIC_LIBS "${zlib_STATIC_LIBRARY}")
        endif()
    endif()

    # Forcing CMake to use its own find module called FindZLIB.cmake.
    # Save old value of CMAKE_MODULE_PATH 
    set(_ZLIB__CMAKE_MODULE_PATH_OLD_ ${CMAKE_MODULE_PATH})
    # Force find_package to use CMAKE module and not custom modules.
    set(CMAKE_MODULE_PATH "${CMAKE_ROOT}/Modules") 

    # Use CMake FindZLIB module. (ZLIB in capital letters is important)
    # FindZLIB supports ZLIB_ROOT, ZLIB_LIBRARIES and ZLIB_INCLUDE_DIRS.
    find_package(ZLIB ${zlib_FIND_VERSION} QUIET)

    # Restore CMAKE_MODULE_PATH
    set(CMAKE_MODULE_PATH ${_ZLIB__CMAKE_MODULE_PATH_OLD_})

    if (ZLIB_FOUND)
        # Right now, OCIO custom find modules uses the following standard: 
        # <pkg_name>_LIBRARY and <pkg_name>_INCLUDE_DIR
        # But CMake's FindZLIB sets ZLIB_LIBRARIES and ZLIB_INCLUDE_DIRS.

        # Set ZLIB_LIBRARY if it is not set already.
        if(ZLIB_LIBRARIES AND NOT ZLIB_LIBRARY)
            set(ZLIB_LIBRARY "${ZLIB_LIBRARIES}")
        endif()

        # Set ZLIB_INCLUDE_DIR if it is not set already.
        if(ZLIB_INCLUDE_DIRS AND NOT ZLIB_INCLUDE_DIR)
            set(ZLIB_INCLUDE_DIR "${ZLIB_INCLUDE_DIRS}")
        endif()
        
        # CMake FindZLIB uses ZLIB_VERSION_STRING for CMake < 3.26. 
        if (ZLIB_VERSION_STRING)
            set(ZLIB_VERSION "${ZLIB_VERSION_STRING}")
        endif()
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(zlib_FIND_REQUIRED FALSE)
        set(ZLIB_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(zlib
        REQUIRED_VARS
            ZLIB_LIBRARY
            ZLIB_INCLUDE_DIR
        VERSION_VAR
            ZLIB_VERSION
    )
endif()

###############################################################################
### Create target

if(NOT TARGET ZLIB::ZLIB)
    add_library(ZLIB::ZLIB UNKNOWN IMPORTED GLOBAL)
    set(_ZLIB_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###
if(NOT ZLIB_FOUND AND OCIO_INSTALL_EXT_PACKAGES AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    if(WIN32)
        set(_ZLIB_LIB_NAME "zlib")
        set(_ZLIB_STATIC_LIB_NAME "zlibstatic")
    else()
        set(_ZLIB_LIB_NAME "z")
        set(_ZLIB_STATIC_LIB_NAME "z")
    endif()

    # Set find_package standard args
    set(ZLIB_FOUND TRUE)
    if(_ZLIB_ExternalProject_VERSION)
        set(ZLIB_VERSION ${_ZLIB_ExternalProject_VERSION})
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

    add_dependencies(ZLIB::ZLIB ZLIB_install)
    
    # Setting those variables to follow the same naming as the other OCIO custom find modules.
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