# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install yaml-cpp
#
# Variables defined by this module:
#   YAMLCPP_FOUND - If FALSE, do not try to link to yamlcpp
#   YAMLCPP_LIBRARY - Where to find yaml-cpp
#   YAMLCPP_INCLUDE_DIR - Where to find yaml.h
#   YAMLCPP_VERSION - The version of the library
#
# Targets defined by this module:
#   yamlcpp::yamlcpp - IMPORTED target, if found
#
# By default, the dynamic libraries of yaml-cpp will be found. To find the 
# static ones instead, you must set the YAMLCPP_STATIC_LIBRARY variable to 
# TRUE before calling find_package(YamlCpp ...).
#
# If yaml-cpp is not installed in a standard path, you can use the 
# YAMLCPP_DIRS variable to tell CMake where to find it. If it is not found 
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, yaml-cpp will be 
# downloaded, built, and statically-linked into libOpenColorIO at build time.
#

if(NOT TARGET yamlcpp::yamlcpp)
    add_library(yamlcpp::yamlcpp UNKNOWN IMPORTED GLOBAL)
    set(_YAMLCPP_TARGET_CREATE TRUE)
endif()

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    # Try to use pkg-config to get the version
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_YAMLCPP QUIET yaml-cpp)

    set(_YAMLCPP_SEARCH_DIRS
        ${YAMLCPP_DIRS}
        ~/Library/Frameworks
        /Library/Frameworks
        /usr/local
        /usr
        /sw        # Fink
        /opt/local # DarwinPorts
        /opt/csw   # Blastwave
        /opt
    )

    # Find include directory
    find_path(YAMLCPP_INCLUDE_DIR 
        NAMES
            yaml-cpp/yaml.h
        HINTS
            ${_YAMLCPP_SEARCH_DIRS}
            ${PC_YAMLCPP_INCLUDE_DIRS}
        PATH_SUFFIXES 
            include
            yaml-cpp/include
    )

    # Lib names to search for
    set(_YAMLCPP_LIB_NAMES yaml-cpp)
    if(WIN32 AND BUILD_TYPE_DEBUG)
        # Prefer Debug lib names (Windows only)
        list(INSERT _YAMLCPP_LIB_NAMES 0 yaml-cppd)
    endif()

    if(YAMLCPP_STATIC_LIBRARY)
        # Prefer static lib names
        if(WIN32)
            set(_YAMLCPP_LIB_SUFFIX "md")
        endif()
        set(_YAMLCPP_STATIC_LIB_NAMES 
            "libyaml-cpp${_YAMLCPP_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
        if(WIN32 AND BUILD_TYPE_DEBUG)
            # Prefer static Debug lib names (Windows only)
            list(INSERT _YAMLCPP_STATIC_LIB_NAMES 0
                "libyaml-cpp${_YAMLCPP_LIB_SUFFIX}d${CMAKE_STATIC_LIBRARY_SUFFIX}")
        endif()
    endif()

    # Find library
    find_library(YAMLCPP_LIBRARY
        NAMES 
            ${_YAMLCPP_STATIC_LIB_NAMES}
            ${_YAMLCPP_LIB_NAMES}
        HINTS 
            ${_YAMLCPP_SEARCH_DIRS}
            ${PC_YAMLCPP_LIBRARY_DIRS}
        PATH_SUFFIXES 
            lib64 lib
    )

    # Get version from config if it was found.
    if(PC_YAMLCPP_VERSION)
        set(YAMLCPP_VERSION "${PC_YAMLCPP_VERSION}")
    elseif(EXISTS "${YAMLCPP_INCLUDE_DIR}/yaml-cpp/iterator.h")
        set(YAMLCPP_VERSION "0.3.0")
    elseif(EXISTS "${YAMLCPP_INCLUDE_DIR}/yaml-cpp/noncopyable.h")
        # The version is higher than 0.3.0 but lower than 0.6.3
        # i.e. the version 0.6.3 removes the file 'yaml-cpp/noncopyable.h.
        set(YAMLCPP_VERSION "0.5.3")
    else()
        # The only supported version.
        set(YAMLCPP_VERSION "0.6.3")
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(YamlCpp_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(YamlCpp
        REQUIRED_VARS 
            YAMLCPP_INCLUDE_DIR 
            YAMLCPP_LIBRARY 
        VERSION_VAR
            YAMLCPP_VERSION
    )
    set(YAMLCPP_FOUND ${YamlCpp_FOUND})
endif()

###############################################################################
### Install package from source ###

if(NOT YAMLCPP_FOUND)
    include(ExternalProject)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(YAMLCPP_FOUND TRUE)
    set(YAMLCPP_VERSION ${YamlCpp_FIND_VERSION})
    set(YAMLCPP_INCLUDE_DIR "${_EXT_DIST_ROOT}/include")

    # Set the expected library name
    if(WIN32)
        set(_YAMLCPP_LIB_SUFFIX "md")
        if(BUILD_TYPE_DEBUG)
            string(APPEND _YAMLCPP_LIB_SUFFIX "d")
        endif()
    endif()
    set(YAMLCPP_LIBRARY 
        "${_EXT_DIST_ROOT}/lib/libyaml-cpp${_YAMLCPP_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_YAMLCPP_TARGET_CREATE)
        if(MSVC)
            set(YAMLCPP_CXX_FLAGS "${YAMLCPP_CXX_FLAGS} /EHsc")
        endif()

        if(UNIX)
            set(YAMLCPP_CXX_FLAGS "${YAMLCPP_CXX_FLAGS} -fvisibility=hidden -fPIC")
            if(OCIO_INLINES_HIDDEN)
                set(YAMLCPP_CXX_FLAGS "${YAMLCPP_CXX_FLAGS} -fvisibility-inlines-hidden")
            endif()
        endif()

        string(STRIP "${YAMLCPP_CXX_FLAGS}" YAMLCPP_CXX_FLAGS)

        set(YAMLCPP_CMAKE_ARGS
            ${YAMLCPP_CMAKE_ARGS}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_SHARED_LIBS:BOOL=OFF
            -DYAML_BUILD_SHARED_LIBS:BOOL=OFF
            -DYAML_CPP_BUILD_TESTS:BOOL=OFF
            -DYAML_CPP_BUILD_TOOLS:BOOL=OFF
            -DYAML_CPP_BUILD_CONTRIB:BOOL=OFF
            -DCMAKE_CXX_FLAGS=${YAMLCPP_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
        )
        if(CMAKE_TOOLCHAIN_FILE)
            set(YAMLCPP_CMAKE_ARGS
                ${YAMLCPP_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(NOT BUILD_SHARED_LIBS)
            #TODO: Find a way to merge in the static libs when built with internal yamlcpp
            message(WARNING
                "Building STATIC libOpenColorIO using the in-built YamlCpp. "
                "YampCpp symbols are NOT included in the output binary!"
            )
        endif()

        set(YAMLCPP_GIT_TAG "yaml-cpp-${YAMLCPP_VERSION}")

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${YAMLCPP_INCLUDE_DIR})

        ExternalProject_Add(yamlcpp_install
            GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp.git"
            GIT_TAG ${YAMLCPP_GIT_TAG}
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/yaml-cpp"
            BUILD_BYPRODUCTS ${YAMLCPP_LIBRARY}
            CMAKE_ARGS ${YAMLCPP_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(yamlcpp::yamlcpp yamlcpp_install)
        message(STATUS
            "Installing YamlCpp: ${YAMLCPP_LIBRARY} (version ${YAMLCPP_VERSION})"
        )
    endif()
endif()

###############################################################################
### Configure target ###

if(_YAMLCPP_TARGET_CREATE)
    set_target_properties(yamlcpp::yamlcpp PROPERTIES
        IMPORTED_LOCATION ${YAMLCPP_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${YAMLCPP_INCLUDE_DIR}
    )

    mark_as_advanced(YAMLCPP_INCLUDE_DIR YAMLCPP_LIBRARY YAMLCPP_VERSION)
endif()
