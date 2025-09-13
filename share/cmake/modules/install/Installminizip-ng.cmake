# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Install minizip-ng
#
# Variables defined by this module:
#   minizip-ng_FOUND          - Indicate whether the library was found or not
#   minizip-ng_LIBRARY        - Path to the library file
#   minizip-ng_INCLUDE_DIR    - Location of the header files
#   minizip-ng_VERSION        - Library's version
#
# Global targets defined by this module:
#   minizip-ng::minizip-ng
#
# Link against the following target:
#   ZLIB::ZLIB
#


###############################################################################
### Create target
if(NOT TARGET MINIZIP::minizip-ng)
    add_library(MINIZIP::minizip-ng UNKNOWN IMPORTED GLOBAL)
    set(_minizip-ng_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT minizip-ng_FOUND AND OCIO_INSTALL_EXT_PACKAGES AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${PROJECT_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${PROJECT_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(minizip-ng_FOUND TRUE)
    if(OCIO_minizip-ng_RECOMMENDED_VERSION)
        set(minizip-ng_VERSION ${OCIO_minizip-ng_RECOMMENDED_VERSION})
    else()
        set(minizip-ng_VERSION ${minizip-ng_FIND_VERSION})
    endif()

    # TODO: Only from a specific version?
    set(minizip-ng_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}/minizip-ng/minizip-ng")

    # Minizip-ng use a hardcoded lib prefix instead of CMAKE_STATIC_LIBRARY_PREFIX
    # Fixed from 4.0.7, see https://github.com/zlib-ng/minizip-ng/issues/778
    if(${minizip-ng_VERSION} VERSION_GREATER_EQUAL "4.0.7")
        set(_minizip-ng_LIB_PREFIX "${CMAKE_STATIC_LIBRARY_PREFIX}")
    else()
        set(_minizip-ng_LIB_PREFIX "lib")
    endif()

    set(minizip-ng_LIBRARY
        "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${_minizip-ng_LIB_PREFIX}minizip-ng${_minizip-ng_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_minizip-ng_TARGET_CREATE)
        set(minizip-ng_CMAKE_ARGS
            ${minizip-ng_CMAKE_ARGS}
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
            # Since the other modules create a subfolder for the includes by default and since
            # minizip-ng does not, a suffix is added to CMAKE_INSTALL_INCLUDEDIR in order to
            # install the headers under a subdirectory named "minizip-ng".
            # Note that this does not affect external builds for minizip-ng.
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
        CMAKE_ARGS ${minizip-ng_CMAKE_ARGS}
        EXCLUDE_FROM_ALL TRUE
        BUILD_COMMAND ""
        INSTALL_COMMAND
            ${CMAKE_COMMAND} --build .
                            --config ${CMAKE_BUILD_TYPE}
                            --target install
                            --parallel
        DEPENDS ZLIB::ZLIB        # minizip-ng depends on zlib
    )

    add_dependencies(MINIZIP::minizip-ng minizip-ng_install)
    if(OCIO_VERBOSE)
        message(STATUS "Installing minizip-ng: ${minizip-ng_LIBRARY} (version \"${minizip-ng_VERSION}\")")
    endif()
endif()

###############################################################################
### Configure target ###

if(_minizip-ng_TARGET_CREATE)
    set_target_properties(MINIZIP::minizip-ng PROPERTIES
        IMPORTED_LOCATION "${minizip-ng_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${minizip-ng_INCLUDE_DIR}"
    )

    mark_as_advanced(minizip-ng_INCLUDE_DIR minizip-ng_LIBRARY minizip-ng_VERSION)

    target_link_libraries(MINIZIP::minizip-ng INTERFACE ZLIB::ZLIB)
endif()
