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
#   yaml-cpp
#
# The libraries files are search based on the CMake variable CMAKE_FIND_LIBRARY_SUFFIXES. On the 
# majority of operating systems, the dynamic libaries are prefered first, and then static libraries.
#
# If your installation has both static and dynamic libraries present, the dynamic one will be used 
# by default. You may set yaml-cpp_STATIC_LIBRARY to TRUE to use the static one. If the static one is 
# missing, CMake won't find anything even if the variable is set. If only the static one is present, 
# that variable is unnecessary. It is ignored if OCIO is installing the library since OCIO builds
# the dependency as static library.
#
# If the library is not installed in a standard path, you can do the following the help
# the find module:
#
# If the package provides a configuration file, use -Dyamp-cppDIR=<path to folder>.
# If it doesn't provide it, try -Dyamp-cppROOT=<path to folder with lib and includes>.
# Alternatively, try -Dyamp-cppLIBRARY=<path to lib file> and -Dyamp-cppINCLUDE_DIR=<path to folder>.
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

        list(APPEND _yaml-cpp_REQUIRED_VARS yaml-cpp_INCLUDE_DIR yaml-cpp_VERSION)

        # Search for yaml-cpp.pc
        find_package(PkgConfig QUIET)
        pkg_check_modules(PC_yaml-cpp QUIET "yaml-cpp>=${yaml-cpp_FIND_VERSION}")

        # Try to detect the version installed, if any.
        if(NOT PC_yaml-cpp_FOUND)
            pkg_search_module(PC_yaml-cpp QUIET "yaml-cpp")
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
endif()

###############################################################################
### Create target

if(yaml-cpp_FOUND AND NOT TARGET yaml-cpp)
    add_library(yaml-cpp UNKNOWN IMPORTED GLOBAL)
    set(_yaml-cpp_TARGET_CREATE TRUE)
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