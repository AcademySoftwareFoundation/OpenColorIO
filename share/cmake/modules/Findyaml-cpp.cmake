# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.
#
# Locate yaml-cpp
#
# Variables defined by this module:
#   yaml-cpp_FOUND          - Indicate whether the library was found or not
#   yaml-cpp_LIBRARY        - Path to the library file
#   yaml-cpp_INCLUDE_DIR    - Location of the header files
#   yaml-cpp_VERSION        - Library's version
#
# Global targets defined by this module:
#   yaml-cpp::yaml-cpp
#
# Usually CMake will use the dynamic library rather than static, if both are present. 
# In this case, you may set yaml-cpp_STATIC_LIBRARY to ON to request use of the static one. 
# If only the static library is present (such as when OCIO builds the dependency), then the option 
# is not needed.
#
# If the library is not installed in a typical location where CMake will find it, you may specify 
# the location using one of the following methods:
# -- Set -Dyaml-cpp_DIR to point to the directory containing the CMake configuration file for the package.
# -- Set -Dyaml-cpp_ROOT to point to the directory containing the lib and include directories.
# -- Set -Dyaml-cpp_LIBRARY and -Dyaml-cpp_INCLUDE_DIR to point to the lib and include directories.
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

if(yaml-cpp_FIND_QUIETLY)
    set(quiet QUIET)
endif()

if(NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL ALL)
    set(_yaml-cpp_REQUIRED_VARS yaml-cpp_LIBRARY)

    # Search for yaml-cpp-config.cmake
    if(NOT DEFINED yaml-cpp_ROOT)
        find_package(yaml-cpp ${yaml-cpp_FIND_VERSION} CONFIG ${quiet})
    endif()

    if(yaml-cpp_FOUND)
        # Alias target for yaml-cpp < 0.8 compatibility
        if(TARGET yaml-cpp AND NOT TARGET yaml-cpp::yaml-cpp)
            add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
        endif()

        get_target_property(yaml-cpp_INCLUDE_DIR yaml-cpp::yaml-cpp INTERFACE_INCLUDE_DIRECTORIES)
        get_target_property(yaml-cpp_LIBRARY yaml-cpp::yaml-cpp LOCATION)
    else()

        # As yaml-cpp-config.cmake search fails, search an installed library
        # using yaml-cpp.pc .

        list(APPEND _yaml-cpp_REQUIRED_VARS yaml-cpp_INCLUDE_DIR yaml-cpp_VERSION)

        # Search for yaml-cpp.pc
        find_package(PkgConfig ${quiet})
        pkg_check_modules(PC_yaml-cpp ${quiet} "yaml-cpp>=${yaml-cpp_FIND_VERSION}")

        # Try to detect the version installed, if any.
        if(NOT PC_yaml-cpp_FOUND)
            pkg_search_module(PC_yaml-cpp ${quiet} "yaml-cpp")
        endif()

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
                YAML_CPP/include
        )

        # Lib names to search for
        set(_yaml-cpp_LIB_NAMES yaml-cpp)
        if(BUILD_TYPE_DEBUG)
            # Prefer Debug lib names.
            list(INSERT _yaml-cpp_LIB_NAMES 0 yaml-cppd)
        endif()

        if(yaml-cpp_STATIC_LIBRARY)
            # Prefer static lib names
            set(_yaml-cpp_STATIC_LIB_NAMES 
                "${CMAKE_STATIC_LIBRARY_PREFIX}yaml-cpp${CMAKE_STATIC_LIBRARY_SUFFIX}")

            # Starting from 0.7.0, all platforms uses the suffix "d" for debug.
            # See https://github.com/jbeder/yaml-cpp/blob/master/CMakeLists.txt#L141
            if(BUILD_TYPE_DEBUG)
                list(INSERT _yaml-cpp_STATIC_LIB_NAMES 0
                    "${CMAKE_STATIC_LIBRARY_PREFIX}yaml-cppd${CMAKE_STATIC_LIBRARY_SUFFIX}")
            endif()
        endif()

        # Find library
        find_library(yaml-cpp_LIBRARY
            NAMES 
                ${_yaml-cpp_STATIC_LIB_NAMES}
                ${_yaml-cpp_LIB_NAMES}
            HINTS 
                ${yaml-cpp_ROOT}
                ${PC_yaml-cpp_LIBRARY_DIRS}
            PATH_SUFFIXES 
                lib64
                lib
                YAML_CPP/lib
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

    mark_as_advanced(yaml-cpp_INCLUDE_DIR yaml-cpp_LIBRARY yaml-cpp_VERSION)
endif()

###############################################################################
### Create target

if (yaml-cpp_FOUND AND NOT TARGET yaml-cpp::yaml-cpp)
    add_library(yaml-cpp::yaml-cpp UNKNOWN IMPORTED GLOBAL)
    set_target_properties(yaml-cpp::yaml-cpp PROPERTIES
        IMPORTED_LOCATION ${yaml-cpp_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${yaml-cpp_INCLUDE_DIR}
    )

    # Required because Installyaml-cpp.cmake creates `yaml-cpp::yaml-cpp`
    # as an alias, and aliases get resolved in exported targets, causing the
    # find_dependency(yaml-cpp) call in OpenColorIOConfig.cmake to fail.
    # This can be removed once Installyaml-cpp.cmake targets yaml-cpp 0.8.
    if (NOT TARGET yaml-cpp)
        add_library(yaml-cpp ALIAS yaml-cpp::yaml-cpp)
    endif ()
endif ()
