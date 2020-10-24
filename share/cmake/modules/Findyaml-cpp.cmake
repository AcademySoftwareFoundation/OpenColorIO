# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate or install yaml-cpp
#
# Variables defined by this module:
#   yaml-cpp_FOUND - If FALSE, do not try to link to yamlcpp
#   yaml-cpp_LIBRARY - yaml-cpp library to link to
#   yaml-cpp_INCLUDE_DIR - Where to find yaml.h
#   yaml-cpp_VERSION - The version of the library
#
# Targets defined by this module:
#   yaml-cpp - IMPORTED target, if found
#
# By default, the dynamic libraries of yaml-cpp will be found. To find the 
# static ones instead, you must set the yaml-cpp_STATIC_LIBRARY variable to 
# TRUE before calling find_package(yaml-cpp ...).
#
# If yaml-cpp is not installed in a standard path, you can use the 
# yaml-cpp_ROOT variable to tell CMake where to find it. If it is not found 
# and OCIO_INSTALL_EXT_PACKAGES is set to MISSING or ALL, yaml-cpp will be 
# downloaded, built, and statically-linked into libOpenColorIO at build time.
#

###############################################################################
### Try to find package ###

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_yaml-cpp_REQUIRED_VARS yaml-cpp_LIBRARY)

    if(NOT DEFINED yaml-cpp_ROOT)
        # Search for yaml-cpp-config.cmake
        find_package(yaml-cpp ${yaml-cpp_FIND_VERSION} CONFIG QUIET)
    endif()

    if(yaml-cpp_FOUND)
        get_target_property(yaml-cpp_LIBRARY yaml-cpp LOCATION)
    else()

        # As yaml-cpp-config.cmake search fails, search an installed library
        # using yaml-cpp.pc .

        list(APPEND _yaml-cpp_REQUIRED_VARS yaml-cpp_INCLUDE_DIR)

        # Search for yaml-cpp.pc
        find_package(PkgConfig QUIET)
        pkg_check_modules(PC_yaml-cpp QUIET "yaml-cpp>=${yaml-cpp_FIND_VERSION}")
        
        # Find include directory
        find_path(yaml-cpp_INCLUDE_DIR 
            NAMES
                yaml-cpp/yaml.h
            HINTS
                ${yaml-cpp_ROOT}
                ${PC_yaml-cpp_INCLUDE_DIRS}
            PATH_SUFFIXES 
                include
                yaml-cpp/include
        )

        # Lib names to search for
        set(_yaml-cpp_LIB_NAMES yaml-cpp)
        if(WIN32 AND BUILD_TYPE_DEBUG)
            # Prefer Debug lib names (Windows only)
            list(INSERT _yaml-cpp_LIB_NAMES 0 yaml-cppd)
        endif()

        if(yaml-cpp_STATIC_LIBRARY)
            # Prefer static lib names
            if(WIN32)
                set(_yaml-cpp_LIB_SUFFIX "md")
            endif()
            set(_yaml-cpp_STATIC_LIB_NAMES 
                "libyaml-cpp${_yaml-cpp_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
            if(WIN32 AND BUILD_TYPE_DEBUG)
                # Prefer static Debug lib names (Windows only)
                list(INSERT _yaml-cpp_STATIC_LIB_NAMES 0
                    "libyaml-cpp${_yaml-cpp_LIB_SUFFIX}d${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endif()
        endif()

        # Find library
        find_library(yaml-cpp_LIBRARY
            NAMES 
                ${_yaml-cpp_STATIC_LIB_NAMES}
                ${_yaml-cpp_LIB_NAMES}
            HINTS 
                ${_yaml-cpp_ROOT}
                ${PC_yaml-cpp_LIBRARY_DIRS}
            PATH_SUFFIXES 
                lib64 lib
        )

        # Get version from pkg-config if it was found.
        if(PC_yaml-cpp_FOUND)
            set(yaml-cpp_VERSION "${PC_yaml-cpp_VERSION}")
        endif()
    endif()

    # Override REQUIRED if package can be installed
    if(OCIO_INSTALL_EXT_PACKAGES STREQUAL MISSING)
        set(yaml-cpp_FIND_REQUIRED FALSE)
    endif()

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(yaml-cpp
        REQUIRED_VARS 
            ${_yaml-cpp_REQUIRED_VARS}
        VERSION_VAR
            yaml-cpp_VERSION
    )
endif()

###############################################################################
### Create target (if previous 'find_package' call hasn't) ###

if(NOT TARGET yaml-cpp)
    add_library(yaml-cpp UNKNOWN IMPORTED GLOBAL)
    set(_yaml-cpp_TARGET_CREATE TRUE)
endif()

###############################################################################
### Install package from source ###

if(NOT yaml-cpp_FOUND)

    # As searches using yaml-cpp-config.cmake and yaml-cpp.pc failed, it now
    # installs the library from the yaml-cpp source.

    include(ExternalProject)

    if(APPLE)
        set(CMAKE_OSX_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})
    endif()

    # TODO: yaml-cpp master is using GNUInstallDirs to define include and lib 
    #       dir names. Once that change is released and OCIO updates the 
    #       minimum yaml-cpp version, toggle the three disabled lines below.
    #include(GNUInstallDirs)

    set(_EXT_DIST_ROOT "${CMAKE_BINARY_DIR}/ext/dist")
    set(_EXT_BUILD_ROOT "${CMAKE_BINARY_DIR}/ext/build")

    # Set find_package standard args
    set(yaml-cpp_FOUND TRUE)
    set(yaml-cpp_VERSION ${yaml-cpp_FIND_VERSION})
    set(yaml-cpp_INCLUDE_DIR "${_EXT_DIST_ROOT}/include")
    #set(yaml-cpp_INCLUDE_DIR "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_INCLUDEDIR}")

    # Set the expected library name
    if(WIN32)
        set(_yaml-cpp_LIB_SUFFIX "md")
        if(BUILD_TYPE_DEBUG)
            string(APPEND _yaml-cpp_LIB_SUFFIX "d")
        endif()
    endif()
    set(yaml-cpp_LIBRARY
        "${_EXT_DIST_ROOT}/lib/libyaml-cpp${_yaml-cpp_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    # set(yaml-cpp_LIBRARY
    #     "${_EXT_DIST_ROOT}/${CMAKE_INSTALL_LIBDIR}/libyaml-cpp${_yaml-cpp_LIB_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}")

    if(_yaml-cpp_TARGET_CREATE)
        if(MSVC)
            set(yaml-cpp_CXX_FLAGS "${yaml-cpp_CXX_FLAGS} /EHsc")
        endif()

        if(UNIX)
            set(yaml-cpp_CXX_FLAGS "${yaml-cpp_CXX_FLAGS} -fvisibility=hidden -fPIC")
            if(OCIO_INLINES_HIDDEN)
                set(yaml-cpp_CXX_FLAGS "${yaml-cpp_CXX_FLAGS} -fvisibility-inlines-hidden")
            endif()
        endif()

        string(STRIP "${yaml-cpp_CXX_FLAGS}" yaml-cpp_CXX_FLAGS)

        set(yaml-cpp_CMAKE_ARGS
            ${yaml-cpp_CMAKE_ARGS}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${yaml-cpp_CXX_FLAGS}
            -DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}
            -DCMAKE_INSTALL_MESSAGE=${CMAKE_INSTALL_MESSAGE}
            -DCMAKE_INSTALL_PREFIX=${_EXT_DIST_ROOT}
            -DCMAKE_OBJECT_PATH_MAX=${CMAKE_OBJECT_PATH_MAX}
            -DBUILD_SHARED_LIBS=OFF
            -DYAML_BUILD_SHARED_LIBS=OFF
            -DYAML_CPP_BUILD_TESTS=OFF
            -DYAML_CPP_BUILD_TOOLS=OFF
            -DYAML_CPP_BUILD_CONTRIB=OFF
        )
        if(CMAKE_TOOLCHAIN_FILE)
            set(yaml-cpp_CMAKE_ARGS
                ${yaml-cpp_CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE})
        endif()

        if(NOT BUILD_SHARED_LIBS)
            #TODO: Find a way to merge in the static libs when built with internal yamlcpp
            message(WARNING
                "Building STATIC libOpenColorIO using the in-built yaml-cpp. "
                "yaml-cpp symbols are NOT included in the output binary!"
            )
        endif()

        set(yaml-cpp_GIT_TAG "yaml-cpp-${yaml-cpp_VERSION}")

        # Hack to let imported target be built from ExternalProject_Add
        file(MAKE_DIRECTORY ${yaml-cpp_INCLUDE_DIR})

        ExternalProject_Add(yaml-cpp_install
            GIT_REPOSITORY "https://github.com/jbeder/yaml-cpp.git"
            GIT_TAG ${yaml-cpp_GIT_TAG}
            GIT_CONFIG advice.detachedHead=false
            GIT_SHALLOW TRUE
            PREFIX "${_EXT_BUILD_ROOT}/yaml-cpp"
            BUILD_BYPRODUCTS ${yaml-cpp_LIBRARY}
            CMAKE_ARGS ${yaml-cpp_CMAKE_ARGS}
            EXCLUDE_FROM_ALL TRUE
        )

        add_dependencies(yaml-cpp yaml-cpp_install)
        message(STATUS
            "Installing yaml-cpp: ${yaml-cpp_LIBRARY} (version \"${yaml-cpp_VERSION}\")"
        )
    endif()
endif()

###############################################################################
### Configure target ###

if(_yaml-cpp_TARGET_CREATE)
    set_target_properties(yaml-cpp PROPERTIES
        IMPORTED_LOCATION ${yaml-cpp_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${yaml-cpp_INCLUDE_DIR}
    )

    mark_as_advanced(yaml-cpp_INCLUDE_DIR yaml-cpp_LIBRARY yaml-cpp_VERSION)
endif()
