# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# CMake definition.

cmake_minimum_required(VERSION 3.12)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/share/cmake/utils
    ${CMAKE_SOURCE_DIR}/share/cmake/macros
    ${CMAKE_SOURCE_DIR}/share/cmake/modules
)

set(CMAKE_WARN_DEPRECATED ON)


if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    # The value of this variable should be set prior to the first project() command invocation
    # because it may influence configuration of the toolchain and flags.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version")
endif()


###############################################################################
# Project definition.

project(OpenColorIO 
    VERSION 2.0.0
    DESCRIPTION "OpenColorIO (OCIO) is a complete color management solution"
    HOMEPAGE_URL https://github.com/AcademySoftwareFoundation/OpenColorIO
    LANGUAGES CXX C)

# "dev" for master or "" for any official release
set(OpenColorIO_VERSION_RELEASE_TYPE "dev")

enable_testing()


###############################################################################
# Forbid in-source build.

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR 
            "In-source build not allowed. Please make a new sub-directory and run cmake from there.")
endif()


###############################################################################
# Global CMake options.

# Do not output install messages.
if(NOT CMAKE_INSTALL_MESSAGE)
    set(CMAKE_INSTALL_MESSAGE "NEVER")
endif()

# Change the path max size to avoid problem on Windows.
if(NOT CMAKE_OBJECT_PATH_MAX)
    set(CMAKE_OBJECT_PATH_MAX 300)
endif()


###############################################################################
# Define compilation mode i.e. debug or release

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

if(NOT CMAKE_CONFIGURATION_TYPES)
    # Set the possible values of build type for cmake-gui or ccmake.
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

set(_BUILD_TYPE_DEBUG OFF)
if(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
    set(_BUILD_TYPE_DEBUG ON)
endif()

set(BUILD_TYPE_DEBUG ${_BUILD_TYPE_DEBUG})


###############################################################################
# C++ version configuration

include(Compilers)
include(CppVersion)


###############################################################################
# Components to build

option(BUILD_SHARED_LIBS "Set to OFF to build static libraries" ON)
option(OCIO_BUILD_APPS "Set to OFF to disable command-line apps" ON)
option(OCIO_BUILD_NUKE "Specify whether to build nuke plugins" OFF)
option(OCIO_BUILD_DOCS "Specify whether to build documentation" OFF)
option(OCIO_BUILD_TESTS "Specify whether to build unittests" ON)
option(OCIO_BUILD_GPU_TESTS "Specify whether to build gpu unittests" ON)
option(OCIO_USE_HEADLESS "Specify whether the GPU rendering is headless" OFF)

option(OCIO_BUILD_PYTHON "Specify whether to build python bindings" ON)
option(OCIO_BUILD_JAVA "Specify whether to build java bindings" OFF)

option(OCIO_WARNING_AS_ERROR "Set build error level for CI testing" OFF)


###############################################################################
# Optimization / internal linking preferences

option(OCIO_USE_SSE "Specify whether to enable SSE CPU performance optimizations" ON)
option(OCIO_INLINES_HIDDEN "Specify whether to build with -fvisibility-inlines-hidden" ${UNIX})


###############################################################################
# Supplemental builtins

# Add supplemental built-in transformations such as camera related one.
option(OCIO_ADD_EXTRA_BUILTINS "Specify whether to add supplemental built-in transforms" ON)


###############################################################################
# GPU configuration

include(CheckSupportGL)


###############################################################################
# Define compilation and link flags

include(CompilerFlags)


###############################################################################
# External linking options

# Supported options
set(_EXT_OPTIONS NONE MISSING ALL)
string(REPLACE ";" ", " _EXT_OPTIONS_STR "${_EXT_OPTIONS}")

# Default option
set(_OCIO_INSTALL_EXT_PACKAGES "MISSING")

if(DEFINED OCIO_INSTALL_EXT_PACKAGES)
    # Case insensitive match
    string(TOUPPER "${OCIO_INSTALL_EXT_PACKAGES}" _OCIO_INSTALL_EXT_PACKAGES)
    if(NOT "${_OCIO_INSTALL_EXT_PACKAGES}" IN_LIST _EXT_OPTIONS)
        message(FATAL_ERROR 
                "OCIO_INSTALL_EXT_PACKAGES=${OCIO_INSTALL_EXT_PACKAGES} is unsupported. Supported values are: ${_EXT_OPTIONS_STR}.")
    endif()
endif()

# Overwrite cache variable with normalized case
set(OCIO_INSTALL_EXT_PACKAGES "${_OCIO_INSTALL_EXT_PACKAGES}" CACHE STRING
    "Set the condition for Installing external dependencies" FORCE)

set_property(CACHE OCIO_INSTALL_EXT_PACKAGES PROPERTY STRINGS ${_EXT_OPTIONS})


###############################################################################
# Versioning

if(NOT SOVERSION)
	set(SOVERSION "${OpenColorIO_VERSION_MAJOR}.${OpenColorIO_VERSION_MINOR}" CACHE STRING 
        "Set the SO version in the SO name of the output library")
endif()


###############################################################################
# Namespace

if(NOT OCIO_NAMESPACE)
	set(OCIO_NAMESPACE OpenColorIO CACHE STRING 
        "Specify the master OCIO C++ namespace: Options include OpenColorIO OpenColorIO_<YOURFACILITY> etc.")
endif()

###############################################################################
# Library name custom suffix
# This helps an application that needs to ship a dynamic library OCIO ensure
# that it has a unique name that won't conflict with one elsewhere on the
# system.

set (OCIO_LIBNAME_SUFFIX "" CACHE STRING
     "Specify a suffix to all libraries that are built")


###############################################################################
# Error checking

if(OCIO_BUILD_SHARED AND OCIO_BUILD_STATIC)
	message(FATAL_ERROR 
            "Deprecated options OCIO_BUILD_SHARED and OCIO_BUILD_STATIC cannot both be used at once")
endif()

if(OCIO_BUILD_SHARED)
	message(DEPRECATION 
            "Option OCIO_BUILD_SHARED is deprecated. Please use the cmake standard BUILD_SHARED_LIBS=ON (default ON)")
	if(NOT BUILD_SHARED_LIBS)
		set(BUILD_SHARED_LIBS ON)
	endif()
endif()

if(OCIO_BUILD_STATIC)
	message(DEPRECATION 
            "Option OCIO_BUILD_STATIC is deprecated. Please use the cmake standard BUILD_SHARED_LIBS=OFF (default ON)")
	if(BUILD_SHARED_LIBS)
		set(BUILD_SHARED_LIBS OFF)
	endif()
endif()


###############################################################################
# Find or install external dependencies
# Some required targets may be created by third-party CMake configs, which 
# don't generally produce global targets. To guarantee all imported targets are 
# global, this module is included at the project root level.

include(FindExtPackages)


###############################################################################
# Progress to other sources

add_subdirectory(ext)
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(tests)
if(OCIO_BUILD_DOCS)
	add_subdirectory(docs)
endif()
add_subdirectory(vendor)
