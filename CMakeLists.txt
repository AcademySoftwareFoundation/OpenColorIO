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


if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    # The value of this variable should be set prior to the first project() command invocation
    # because it may influence configuration of the toolchain and flags.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment version")
endif()


###############################################################################
# Project definition.

project(OpenColorIO 
    VERSION 2.2.0
    DESCRIPTION "OpenColorIO (OCIO) is a complete color management solution"
    HOMEPAGE_URL https://github.com/AcademySoftwareFoundation/OpenColorIO
    LANGUAGES CXX C)

# "dev", "beta1", rc1", etc or "" for an official release.
set(OpenColorIO_VERSION_RELEASE_TYPE "")


###############################################################################
# ctest

# To correctly generate the DartConfiguration.tcl, include the CTest CMake file.
# By doing so 'ctest -F memcheck' works.
include (CTest)

enable_testing()


###############################################################################
# Provides install directory variables as defined by the GNU Coding Standards.
include(GNUInstallDirs)


###############################################################################
# Forbid in-source build.

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR 
            "In-source build not allowed. Please make a new sub-directory and run cmake from there.")
endif()


###############################################################################
# Global CMake options.

# Do not output install messages.
if(NOT DEFINED CMAKE_INSTALL_MESSAGE)
    set(CMAKE_INSTALL_MESSAGE "NEVER")
endif()

# Change the path max size to avoid problem on Windows.
if(NOT DEFINED CMAKE_OBJECT_PATH_MAX)
    set(CMAKE_OBJECT_PATH_MAX 300)
endif()


###############################################################################
# Define compilation mode i.e. debug or release

if(NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# List all the valid build types.

if(NOT DEFINED CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release;MinSizeRel;RelWithDebInfo" CACHE STRING "" FORCE)
    mark_as_advanced(CMAKE_CONFIGURATION_TYPES)
endif()

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")

# Is that a valid build type?

if(NOT "${CMAKE_BUILD_TYPE}" IN_LIST CMAKE_CONFIGURATION_TYPES)
    string(REPLACE ";" ", " _CMAKE_CONFIGURATION_TYPES_STR "${CMAKE_CONFIGURATION_TYPES}")
    message(FATAL_ERROR 
            "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} is unsupported. Supported values are: ${_CMAKE_CONFIGURATION_TYPES_STR}.")
endif()

# Is that in debug mode?

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
option(OCIO_BUILD_OPENFX "Specify whether to build OpenFX plugins" OFF)
option(OCIO_BUILD_NUKE "Specify whether to build nuke plugins" OFF)
option(OCIO_BUILD_TESTS "Specify whether to build unittests" ON)
option(OCIO_BUILD_GPU_TESTS "Specify whether to build gpu unittests" ON)
option(OCIO_USE_HEADLESS "Specify whether the GPU rendering is headless" OFF)

option(OCIO_BUILD_FROZEN_DOCS "Specify whether to build frozen documentation, necessary when modifying the OCIO public API" OFF)
option(OCIO_BUILD_DOCS "Specify whether to build documentation" ${OCIO_BUILD_FROZEN_DOCS})

option(OCIO_BUILD_PYTHON "Specify whether to build python bindings" ON)
set (OCIO_PYTHON_VERSION "" CACHE STRING
     "Preferred Python version (if any) in case multiple are available")

option(OCIO_BUILD_JAVA "Specify whether to build java bindings" OFF)

option(OCIO_WARNING_AS_ERROR "Set build error level for CI testing" OFF)

if (WIN32)
    option(OCIO_USE_WINDOWS_UNICODE "Compile with Windows Unicode support" ON)
endif()


###############################################################################
# Optimization / internal linking preferences

option(OCIO_USE_SSE "Specify whether to enable SSE CPU performance optimizations" ON)
option(OCIO_USE_OIIO_CMAKE_CONFIG "Specify whether to look for OIIO using the generated CMake Config script instead of the custom FindOpenImageIO.cmake script" OFF)
option(OCIO_USE_OIIO_FOR_APPS "Request OIIO to build apps (ociolutimage, ocioconvert and ociodisplay), the default uses OpenEXR." OFF)


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

if(NOT DEFINED SOVERSION)
    set(SOVERSION "${OpenColorIO_VERSION_MAJOR}.${OpenColorIO_VERSION_MINOR}" CACHE STRING 
        "Set the SO version in the SO name of the output library")
    message(STATUS "Setting SOVERSION to '${SOVERSION}' as none was specified.")
elseif(SOVERSION STREQUAL "")
    message(FATAL_ERROR "A SOVERSION cannot be empty.")
endif()


###############################################################################
# Namespace

if(NOT DEFINED OCIO_NAMESPACE)
	set(OCIO_NAMESPACE "OpenColorIO_v${OpenColorIO_VERSION_MAJOR}_${OpenColorIO_VERSION_MINOR}${OpenColorIO_VERSION_RELEASE_TYPE}" CACHE STRING 
        "Specify the main OCIO C++ namespace: Options include OpenColorIO OpenColorIO_<YOURFACILITY> etc.")
elseif(OCIO_NAMESPACE STREQUAL "")
    message(FATAL_ERROR "A namespace cannot be empty.")
else()
    set(OCIO_NAMESPACE "OpenColorIO_${OCIO_NAMESPACE}_v${OpenColorIO_VERSION_MAJOR}_${OpenColorIO_VERSION_MINOR}${OpenColorIO_VERSION_RELEASE_TYPE}" CACHE STRING 
        "Specify the main OCIO C++ namespace: Options include OpenColorIO OpenColorIO_<YOURFACILITY> etc.")
    message(STATUS "Setting namespace to '${OCIO_NAMESPACE}' as none was specified.")
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

add_subdirectory(vendor)
if(OCIO_BUILD_DOCS)
	add_subdirectory(docs)
endif()
add_subdirectory(tests)
add_subdirectory(src)
add_subdirectory(ext)


###############################################################################
# Configure env script

if(WIN32)
    set(OCIO_SETUP_NAME setup_ocio.bat)
else()
    set(OCIO_SETUP_NAME setup_ocio.sh)
endif()

configure_file(${CMAKE_SOURCE_DIR}/share/ocio/${OCIO_SETUP_NAME}.in
    ${CMAKE_CURRENT_BINARY_DIR}/share/ocio/${OCIO_SETUP_NAME} @ONLY)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/share/ocio/${OCIO_SETUP_NAME} DESTINATION ${CMAKE_INSTALL_DATADIR}/ocio/)


###############################################################################
# Generate OpenColorIO-config.cmake
# Targets exported to ${PROJECT_NAME}_EXPORTED_TARGETS will be available for
# import using find_package(OpenColorIO).

include(CMakePackageConfigHelpers)

set(OCIO_TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets.cmake")
set(OCIO_VERSION_CONFIG "${CMAKE_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(OCIO_PROJECT_CONFIG "${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
set(OCIO_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

# Version fetched from the top level project()
write_basic_package_version_file(
    ${OCIO_VERSION_CONFIG}
    # Version range supported only if generated by CMake 3.19.2 or newer.
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    ${CMAKE_CURRENT_LIST_DIR}/src/cmake/Config.cmake.in ${OCIO_PROJECT_CONFIG}
    INSTALL_DESTINATION ${OCIO_CONFIG_INSTALL_DIR}
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

install(
    EXPORT ${PROJECT_NAME}_EXPORTED_TARGETS
    DESTINATION ${OCIO_CONFIG_INSTALL_DIR}
    NAMESPACE ${PROJECT_NAME}::
    FILE ${OCIO_TARGETS_EXPORT_NAME}
)

install(
    FILES "${OCIO_PROJECT_CONFIG}" "${OCIO_VERSION_CONFIG}"
    DESTINATION "${OCIO_CONFIG_INSTALL_DIR}"
)
