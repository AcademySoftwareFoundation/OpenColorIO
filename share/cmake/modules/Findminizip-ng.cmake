# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install minizip-ng
#
# Variables defined by this module:
#   minizip-ng_FOUND        - If FALSE, do not try to link to minizip-ng
#   minizip-ng_LIBRARY      - minizip-ng library to link to
#   minizip-ng_INCLUDE_DIR  - Where to find mz.h and other headers
#   minizip-ng_VERSION      - The version of the library
#
# Targets defined by this module:
#   minizip-ng::minizip-ng - IMPORTED target, if found
#
###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    if(NOT DEFINED minizip-ng_ROOT)
        # Search for minizip-ng-config.cmake
        find_package(minizip-ng ${minizip-ng_FIND_VERSION} CONFIG)
    endif()

    if (minizip-ng_FOUND)
        get_target_property(minizip-ng_LIBRARY MINIZIP::minizip-ng LOCATION)
        get_target_property(minizip-ng_INCLUDE_DIR MINIZIP::minizip-ng INTERFACE_INCLUDE_DIRECTORIES)
    else ()
        list(APPEND _minizip-ng_REQUIRED_VARS minizip-ng_INCLUDE_DIR)

        # Search for minizip-ng.pc
        find_package(PkgConfig QUIET)
        pkg_check_modules(PC_minizip-ng QUIET "minizip-ng>=${minizip-ng_FIND_VERSION}")

        # Find include directory
        find_path(minizip-ng_INCLUDE_DIR
            NAMES
                mz.h
            HINTS
                ${minizip-ng_ROOT}
                ${PC_minizip-ng_INCLUDE_DIRS}
            PATH_SUFFIXES
                include
                minizip-ng/include
        )

        # Lib names to search for
        set(_minizip-ng_LIB_NAMES minizip-ng libminizip-ng)
        if(BUILD_TYPE_DEBUG)
            # Prefer Debug lib names (Windows only)
            list(INSERT _minizip-ng_LIB_NAMES 0 minizip-ngd)
        endif()

        if(minizip-ng_STATIC_LIBRARY)
            # Prefer static lib names
            set(_minizip-ng_STATIC_LIB_NAMES 
                "${CMAKE_STATIC_LIBRARY_PREFIX}minizip-ng${CMAKE_STATIC_LIBRARY_SUFFIX}")
            if(WIN32 AND BUILD_TYPE_DEBUG)
                # Prefer static Debug lib names (Windows only)
                list(INSERT _minizip-ng_STATIC_LIB_NAMES 0
                    "${CMAKE_STATIC_LIBRARY_PREFIX}minizip-ngd${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endif()
        endif()

        # Find library
        find_library(minizip-ng_LIBRARY
            NAMES
                ${_minizip-ng_STATIC_LIB_NAMES}
                ${_minizip-ng_LIB_NAMES}
            HINTS
                ${minizip-ng_ROOT}
                ${PC_minizip-ng_LIBRARY_DIRS}
            PATH_SUFFIXES
                lib64 lib 
        )

        # Get version from header or pkg-config
        set(minizip-ng_VERSION "${minizip-ng_FIND_VERSION}")
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(minizip-ng_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(minizip-ng
        REQUIRED_VARS 
            minizip-ng_LIBRARY
            minizip-ng_INCLUDE_DIR
        VERSION_VAR
            minizip-ng_VERSION
    )
endif()

###############################################################################
### Create target

if(NOT TARGET minizip-ng::minizip-ng)
    add_library(minizip-ng::minizip-ng UNKNOWN IMPORTED GLOBAL)
    set(_minizip-ng_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###
if(NOT minizip-ng_FOUND AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(minizip-ng_FOUND TRUE)
    set(minizip-ng_VERSION ${minizip-ng_FIND_VERSION})
    set(minizip-ng_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    # Minizip-ng use a hardcoded lib prefix instead of CMAKE_STATIC_LIBRARY_PREFIX
    set(_minizip-ng_LIB_PREFIX "lib")

    set(minizip-ng_LIBRARY
        "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${_minizip-ng_LIB_PREFIX}minizip-ng${_minizip-ng_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_minizip-ng_TARGET_CREATE)
        set(MINIZIP-NG_CMAKE_ARGS
            ${MINIZIP-NG_CMAKE_ARGS}
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
            -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}/minizip-ng
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DBUILD_SHARED_LIBS=OFF
            -DMZ_OPENSSL=OFF
            -DMZ_LIBBSD=OFF
            -DMZ_BUILD_TESTS=OFF
            -DMZ_COMPAT=OFF
            -DMZ_BZIP2=OFF
            -DMZ_LZMA=OFF
            -DMZ_LIBCOMP=OFF
            -DMZ_ZSTD=OFF
            -DMZ_PKCRYPT=OFF
            -DMZ_WZAES=OFF
            -DMZ_SIGNING=OFF
            -DMZ_ZLIB=ON
            -DMZ_ICONV=OFF
            -DMZ_FETCH_LIBS=OFF
            -DMZ_FORCE_FETCH_LIBS=OFF
            -DZLIB_LIBRARY=${ZLIB_LIBRARIES}
            -DZLIB_INCLUDE_DIR=${ZLIB_INCLUDE_DIRS}
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(minizip-ng_CMAKE_ARGS
                ${minizip-ng_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            string(REPLACE ";" "$<SEMICOLON>" ESCAPED_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

            set(minizip-ng_CMAKE_ARGS
                ${minizip-ng_CMAKE_ARGS}
                -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_OSX_ARCHITECTURES=${ESCAPED_CMAKE_OSX_ARCHITECTURES}
            )
        endif()

        if (ANDROID)
            set(minizip-ng_CMAKE_ARGS
                ${minizip-ng_CMAKE_ARGS}
                -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_STL=${ANDROID_STL})
        endif()
    endif()

    # Hack to let imported target be built from ExternalProject_Add
    file(MAKE_DIRECTORY ${minizip-ng_INCLUDE_DIR})

    ExternalProject_Add(minizip-ng_install
        GIT_REPOSITORY "https://github.com/zlib-ng/minizip-ng.git"
        GIT_TAG "${minizip-ng_VERSION}"
        GIT_CONFIG advice.detachedHead=false
        GIT_SHALLOW TRUE
        PREFIX "${_EXT_BUILD_ROOT}/libminizip-ng"
        BUILD_BYPRODUCTS ${minizip-ng_LIBRARY}
        CMAKE_ARGS ${MINIZIP-NG_CMAKE_ARGS}
        EXCLUDE_FROM_ALL TRUE
        BUILD_COMMAND ""
        INSTALL_COMMAND
            ${CMAKE_COMMAND} --build .
                             --config ${CMAKE_BUILD_TYPE}
                             --target install
                             --parallel
    )

    add_dependencies(minizip-ng::minizip-ng minizip-ng_install)
    message(STATUS "Installing minizip-ng: ${minizip-ng_LIBRARY} (version \"${minizip-ng_VERSION}\")")
endif()

###############################################################################
### Configure target ###

if(_minizip-ng_TARGET_CREATE)
    set_target_properties(minizip-ng::minizip-ng PROPERTIES
        IMPORTED_LOCATION "${minizip-ng_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${minizip-ng_INCLUDE_DIR}"
    )

    mark_as_advanced(minizip-ng_INCLUDE_DIR minizip-ng_LIBRARY minizip-ng_VERSION)

    target_link_libraries(minizip-ng::minizip-ng INTERFACE ZLIB::ZLIB)
endif()