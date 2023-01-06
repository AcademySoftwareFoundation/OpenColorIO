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
#   MINIZIP::minizip-ng - IMPORTED target, if found
#
# If minizip-ng is not installed in a standard path, you can use the minizip-ng_ROOT 
# variable to tell CMake where to find it. If it is not found and 
# OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, minizip-ng will be downloaded, 
# built, and statically-linked into libOpenColorIO at build time.
#
# For external builds of minizip-ng, please note that the same build options should be used.
# Using more options, such as enabling other compression methods, will provoke linking issue
# since OCIO is not linking to those libraries.
#
# e.g. Setting MZ_BZIP2=ON will cause linking issue since OCIO will not be linked against BZIP2.
#
###############################################################################
### Try to find package ###

# BUILD_TYPE_DEBUG variable is currently set in one of the OCIO's CMake files. 
# Now that some OCIO's find module are installed with the library itself (with static build), 
# a consumer app don't have access to the variables set by an OCIO's CMake files. Therefore, some 
# OCIO's find modules must detect the build type by itselves. 
set(BUILD_TYPE_DEBUG OFF)
if(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
   set(BUILD_TYPE_DEBUG ON)
endif()

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    if(NOT DEFINED minizip-ng_ROOT)
        # Search for minizip-ng-config.cmake
        find_package(minizip-ng ${minizip-ng_FIND_VERSION} CONFIG QUIET)
    endif()

    if (minizip-ng_FOUND)
        get_target_property(minizip-ng_INCLUDE_DIR MINIZIP::minizip-ng INTERFACE_INCLUDE_DIRECTORIES)
        get_target_property(minizip-ng_LIBRARY MINIZIP::minizip-ng LOCATION)
        
        # Depending on the options used when minizip-ng was built, it could have multiple libraries
        # listed in INTERFACE_LINK_LIBRARIES. OCIO only needs ZLIB.
        # Only add custom zlib target ZLIB::ZLIB to INTERFACE_LINK_LIBRARIES.
        set_target_properties(MINIZIP::minizip-ng PROPERTIES INTERFACE_LINK_LIBRARIES "ZLIB::ZLIB")

        if (NOT minizip-ng_LIBRARY)
            # Lib names to search for
            set(_minizip-ng_LIB_NAMES minizip-ng)

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

            # Set IMPORTED_LOCATION property for MINIZIP::minizip-ng target.
            if (TARGET MINIZIP::minizip-ng)
                set_target_properties(MINIZIP::minizip-ng PROPERTIES
                    IMPORTED_LOCATION "${minizip-ng_LIBRARY}"
                )
            endif()
        endif()
    else()
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
                include/minizip-ng
                minizip-ng/include
        )

        # Minizip-ng uses prefix "lib" on all platform by default.
        # Library name doesn't change in debug.

        # Lib names to search for.
        # Search for "minizip" since the library could be named "minizip" if it was built 
        # with MZ_COMPAT=ON.
        set(_minizip-ng_LIB_NAMES libminizip-ng minizip-ng libminizip minizip)

        if(minizip-ng_STATIC_LIBRARY)
            # Looking for both "lib" prefix and CMAKE_STATIC_LIBRARY_PREFIX.
            # Prefer static lib names.
            set(_minizip-ng_STATIC_LIB_NAMES 
                "libminizip-ng${CMAKE_STATIC_LIBRARY_SUFFIX}"
                "${CMAKE_STATIC_LIBRARY_PREFIX}minizip-ng${CMAKE_STATIC_LIBRARY_SUFFIX}"
                "libminizip${CMAKE_STATIC_LIBRARY_SUFFIX}"
                "${CMAKE_STATIC_LIBRARY_PREFIX}minizip${CMAKE_STATIC_LIBRARY_SUFFIX}")
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
        if(minizip-ng_INCLUDE_DIR)
            list(GET minizip-ng_INCLUDE_DIR 0 _minizip-ng_INCLUDE_DIR)
            if(EXISTS "${_minizip-ng_INCLUDE_DIR}/mz.h")
                set(_minizip-ng_CONFIG "${_minizip-ng_INCLUDE_DIR}/mz.h")
            endif()
        endif()

        if(_minizip-ng_CONFIG)
            file(STRINGS "${_minizip-ng_CONFIG}" _minizip-ng_VER_SEARCH 
                REGEX "^[ \t]*#define[ \t]+MZ_VERSION[ \t]+\\(\"[.0-9]+\"\\).*$")
            if(_minizip-ng_VER_SEARCH)
                string(REGEX REPLACE ".*#define[ \t]+MZ_VERSION[ \t]+\\(\"([.0-9]+)\"\\).*" 
                    "\\1" minizip-ng_VERSION "${_minizip-ng_VER_SEARCH}")
            endif()
        elseif(PC_minizip-ng_FOUND)
            set(minizip-ng_VERSION "${PC_minizip-ng_VERSION}")
        endif()
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
if(NOT TARGET MINIZIP::minizip-ng)
    add_library(MINIZIP::minizip-ng UNKNOWN IMPORTED GLOBAL)
    set(_minizip-ng_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT minizip-ng_FOUND AND OCIO_INSTALL_EXT_PACKAGES AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(minizip-ng_FOUND TRUE)
    set(minizip-ng_VERSION ${minizip-ng_FIND_VERSION})

    set(minizip-ng_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}/minizip-ng")

    # Minizip-ng use a hardcoded lib prefix instead of CMAKE_STATIC_LIBRARY_PREFIX
    set(_minizip-ng_LIB_PREFIX "lib")

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
    )

    add_dependencies(MINIZIP::minizip-ng minizip-ng_install)
    message(STATUS "Installing minizip-ng: ${minizip-ng_LIBRARY} (version \"${minizip-ng_VERSION}\")")
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