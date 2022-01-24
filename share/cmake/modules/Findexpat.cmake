# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install expat
#
# Variables defined by this module:
#   expat_FOUND - If FALSE, do not try to link to expat
#   expat_LIBRARY - expat library to link to
#   expat_INCLUDE_DIR - Where to find expat.h
#   expat_VERSION - The version of the library
#
# Targets defined by this module:
#   expat::expat - IMPORTED target, if found
#
# By default, the dynamic libraries of expat will be found. To find the static 
# ones instead, you must set the expat_STATIC_LIBRARY variable to TRUE 
# before calling find_package(expat ...).
#
# If expat is not installed in a standard path, you can use the expat_ROOT 
# variable to tell CMake where to find it. If it is not found and 
# OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, expat will be downloaded, 
# built, and statically-linked into libOpenColorIO at build time.
#

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_expat_REQUIRED_VARS expat_LIBRARY)

    if(NOT DEFINED expat_ROOT)
        # Search for expat-config.cmake
        find_package(expat ${expat_FIND_VERSION} CONFIG QUIET)
    endif()

    if(expat_FOUND)
        if (TARGET expat::libexpat)
            message(STATUS "Expat ${expat_VERSION} detected, aliasing targets.")
            add_library(expat::expat ALIAS expat::libexpat)
        endif()

        get_target_property(expat_INCLUDE_DIR expat::expat INTERFACE_INCLUDE_DIRECTORIES)

        get_target_property(expat_LIBRARY expat::expat LOCATION)

        if (NOT expat_INCLUDE_DIR)
            # Find include directory too, as its Config module doesn't include it
            find_path(expat_INCLUDE_DIR
                NAMES
                    expat.h
                HINTS
                    ${expat_ROOT}
                    ${PC_expat_INCLUDE_DIRS}
                PATH_SUFFIXES
                    include
                    expat/include
            )
            message(WARNING "Expat's include directory not specified in its Config module, patching it now to ${expat_INCLUDE_DIR}.")
            if (TARGET expat::libexpat)
                set_target_properties(expat::libexpat PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES ${expat_INCLUDE_DIR}
                )
            else()
                set_target_properties(expat::expat PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES ${expat_INCLUDE_DIR}
                )
            endif()
        endif()

        set(_expat_REQUIRED_VARS ${expat_REQUIRED_VARS} expat_INCLUDE_DIR)
    else()
        list(APPEND _expat_REQUIRED_VARS expat_INCLUDE_DIR)

        # Search for expat.pc
        find_package(PkgConfig QUIET)
        pkg_check_modules(PC_expat QUIET "expat>=${expat_FIND_VERSION}")

        # Find include directory
        find_path(expat_INCLUDE_DIR
            NAMES
                expat.h
            HINTS
                ${expat_ROOT}
                ${PC_expat_INCLUDE_DIRS}
            PATH_SUFFIXES
                include
                expat/include
        )

        # Lib names to search for
        set(_expat_LIB_NAMES expat libexpat)
        if(WIN32 AND BUILD_TYPE_DEBUG)
            # Prefer Debug lib names (Windows only)
            list(INSERT _expat_LIB_NAMES 0 expatd)
        endif()

        if(expat_STATIC_LIBRARY)
            # Prefer static lib names
            set(_expat_STATIC_LIB_NAMES 
                "${CMAKE_STATIC_LIBRARY_PREFIX}expat${CMAKE_STATIC_LIBRARY_SUFFIX}")
            if(WIN32 AND BUILD_TYPE_DEBUG)
                # Prefer static Debug lib names (Windows only)
                list(INSERT _expat_STATIC_LIB_NAMES 0
                    "${CMAKE_STATIC_LIBRARY_PREFIX}expatd${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endif()
        endif()

        # Find library
        find_library(expat_LIBRARY
            NAMES
                ${_expat_STATIC_LIB_NAMES}
                ${_expat_LIB_NAMES}
            HINTS
                ${expat_ROOT}
                ${PC_expat_LIBRARY_DIRS}
            PATH_SUFFIXES
                lib64 lib 
        )

        # Get version from header or pkg-config
        if(expat_INCLUDE_DIR AND EXISTS "${expat_INCLUDE_DIR}/expat.h")
            file(STRINGS "${expat_INCLUDE_DIR}/expat.h" _expat_VER_SEARCH 
                REGEX "^[ \t]*#define[ \t]+XML_(MAJOR|MINOR|MICRO)_VERSION[ \t]+[0-9]+.*$")
            if(_expat_VER_SEARCH)
                foreach(_VER_PART MAJOR MINOR MICRO)
                    string(REGEX REPLACE ".*#define[ \t]+XML_${_VER_PART}_VERSION[ \t]+([0-9]+).*" 
                        "\\1" expat_VERSION_${_VER_PART} "${_expat_VER_SEARCH}")
                    if(NOT expat_VERSION_${_VER_PART})
                        set(expat_VERSION_${_VER_PART} 0)
                    endif()
                endforeach()
                set(expat_VERSION 
                    "${expat_VERSION_MAJOR}.${expat_VERSION_MINOR}.${expat_VERSION_MICRO}")
            endif()
        elseif(PC_expat_FOUND)
            set(expat_VERSION "${PC_expat_VERSION}")
        endif()
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(expat_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(expat
        REQUIRED_VARS 
            ${_expat_REQUIRED_VARS}
        VERSION_VAR
            expat_VERSION
    )
endif()

###############################################################################
### Create target

if(NOT TARGET expat::expat)
    add_library(expat::expat UNKNOWN IMPORTED GLOBAL)
    set(_expat_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT expat_FOUND AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
    include(ExternalProject)
    include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(expat_FOUND TRUE)
    set(expat_VERSION ${expat_FIND_VERSION})
    set(expat_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    # Set the expected library name
    if(WIN32)
        if(BUILD_TYPE_DEBUG)
            set(_expat_LIB_SUFFIX "d")
        endif()
        # Static Linking, Multi-threaded Dll naming (>=2.2.8):
        #   https://github.com/libexpat/libexpat/blob/R_2_2_8/expat/win32/README.txt
        set(_expat_LIB_SUFFIX "${_expat_LIB_SUFFIX}MD")
    endif()

    # Expat use a hardcoded lib prefix instead of CMAKE_STATIC_LIBRARY_PREFIX
    # https://github.com/libexpat/libexpat/blob/R_2_4_1/expat/CMakeLists.txt#L374
    set(_expat_LIB_PREFIX "lib")

    set(expat_LIBRARY
        "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/${_expat_LIB_PREFIX}expat${_expat_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_expat_TARGET_CREATE)
        if(MSVC)
            set(EXPAT_C_FLAGS "${EXPAT_C_FLAGS} /EHsc")
            set(EXPAT_CXX_FLAGS "${EXPAT_CXX_FLAGS} /EHsc")
        endif()

        string(STRIP "${EXPAT_C_FLAGS}" EXPAT_C_FLAGS)
        string(STRIP "${EXPAT_CXX_FLAGS}" EXPAT_CXX_FLAGS)

        set(EXPAT_CMAKE_ARGS
            ${EXPAT_CMAKE_ARGS}
            -DCMAKE_POLICY_DEFAULT_CMP0063=NEW
            -DCMAKE_C_VISIBILITY_PRESET=${CMAKE_C_VISIBILITY_PRESET}
            -DCMAKE_CXX_VISIBILITY_PRESET=${CMAKE_CXX_VISIBILITY_PRESET}
            -DCMAKE_VISIBILITY_INLINES_HIDDEN=${CMAKE_VISIBILITY_INLINES_HIDDEN}
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_C_FLAGS=${EXPAT_C_FLAGS}
            -DCMAKE_CXX_FLAGS=${EXPAT_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
            -DCMAKE_INSTALL_DATADIR=${CMAKE_INSTALL_DATADIR}
            -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
            -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DEXPAT_BUILD_DOCS=OFF
            -DEXPAT_BUILD_EXAMPLES=OFF
            -DEXPAT_BUILD_TESTS=OFF
            -DEXPAT_BUILD_TOOLS=OFF
            -DEXPAT_SHARED_LIBS=OFF
        )

        if(CMAKE_TOOLCHAIN_FILE)
            set(EXPAT_CMAKE_ARGS
                ${EXPAT_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(APPLE)
            string(REPLACE ";" "$<SEMICOLON>" ESCAPED_CMAKE_OSX_ARCHITECTURES "${CMAKE_OSX_ARCHITECTURES}")

            set(EXPAT_CMAKE_ARGS
                ${EXPAT_CMAKE_ARGS}
                -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}
                -DCMAKE_OSX_ARCHITECTURES=${ESCAPED_CMAKE_OSX_ARCHITECTURES}
            )
        endif()

        if (ANDROID)
            set(EXPAT_CMAKE_ARGS
                ${EXPAT_CMAKE_ARGS}
                -DANDROID_PLATFORM=${ANDROID_PLATFORM}
                -DANDROID_ABI=${ANDROID_ABI}
                -DANDROID_STL=${ANDROID_STL})
        endif()

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${expat_INCLUDE_DIR})

        ExternalProject_Add(expat_install
            GIT_REPOSITORY "https://github.com/libexpat/libexpat.git"
            GIT_TAG "R_${expat_FIND_VERSION_MAJOR}_${expat_FIND_VERSION_MINOR}_${expat_FIND_VERSION_PATCH}"
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/libexpat"
            BUILD_BYPRODUCTS ${expat_LIBRARY}
            SOURCE_SUBDIR expat
            CMAKE_ARGS ${EXPAT_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
            BUILD_COMMAND ""
            INSTALL_COMMAND
                ${CMAKE_COMMAND} --build .
                                 --config ${CMAKE_BUILD_TYPE}
                                 --target install
                                 --parallel
        )

        add_dependencies(expat::expat expat_install)
        message(STATUS "Installing expat: ${expat_LIBRARY} (version \"${expat_VERSION}\")")
    endif()
endif()

###############################################################################
### Configure target ###

if(_expat_TARGET_CREATE)
    set_target_properties(expat::expat PROPERTIES
        IMPORTED_LOCATION ${expat_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${expat_INCLUDE_DIR}
    )

    mark_as_advanced(expat_INCLUDE_DIR expat_LIBRARY expat_VERSION)
endif()
