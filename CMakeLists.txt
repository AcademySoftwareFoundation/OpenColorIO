# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# CMake definition.

cmake_minimum_required(VERSION 3.14...4.0)

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/share/cmake/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/share/cmake/macros
    ${CMAKE_CURRENT_SOURCE_DIR}/share/cmake/modules
    ${CMAKE_CURRENT_SOURCE_DIR}/share/cmake/modules/install
)

set(CMAKE_WARN_DEPRECATED ON)


if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    # The value of this variable should be set prior to the first project() command invocation
    # because it may influence configuration of the toolchain and flags.
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
endif()


###############################################################################
# Project definition.

project(OpenColorIO 
    VERSION 2.5.0
    DESCRIPTION "OpenColorIO (OCIO) is a complete color management solution"
    HOMEPAGE_URL https://github.com/AcademySoftwareFoundation/OpenColorIO
    LANGUAGES CXX C)

# "dev", "beta1", rc1", etc or "" for an official release.
set(OpenColorIO_VERSION_RELEASE_TYPE "dev")


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

if(${PROJECT_SOURCE_DIR} STREQUAL ${PROJECT_BINARY_DIR})
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

set(_build_type_valid_ false)
foreach (_build_type_ ${CMAKE_CONFIGURATION_TYPES})
    string(TOLOWER ${_build_type_} _lowercase_build_type_)
    string(TOLOWER ${CMAKE_BUILD_TYPE} _lowercase_cmake_build_type_)

    if (_lowercase_cmake_build_type_ STREQUAL _lowercase_build_type_)
        # Build type is supported.
        set(_build_type_valid_ true)
        break()
    endif()
endforeach()
unset(_lowercase_build_type_)
unset(_lowercase_cmake_build_type_)

if (NOT _build_type_valid_)
    string(REPLACE ";" ", " _CMAKE_CONFIGURATION_TYPES_STR "${CMAKE_CONFIGURATION_TYPES}")
    message(FATAL_ERROR 
            "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} is unsupported. Supported values are: ${_CMAKE_CONFIGURATION_TYPES_STR}.")
endif()
unset(_build_type_valid_)

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

option(OCIO_BUILD_DOCS "Specify whether to build documentation" OFF)

option(OCIO_BUILD_PYTHON "Specify whether to build python bindings" ON)
set (OCIO_PYTHON_VERSION "" CACHE STRING
     "Preferred Python version (if any) in case multiple are available")

option(OCIO_BUILD_JAVA "Specify whether to build java bindings" OFF)

if (WIN32)
    option(OCIO_USE_WINDOWS_UNICODE "Compile with Windows Unicode support" ON)
endif()

###############################################################################
# Other preferences

option(OCIO_VERBOSE "Display more information when searching or installing dependencies" OFF)
option(OCIO_USE_SOVERSION "Enable versioning in OCIO shared library name" ON)

###############################################################################
# Warnings / debugging settings

option(OCIO_WARNING_AS_ERROR "Set build error level for CI testing" OFF)

if (NOT WIN32)
    option(OCIO_ENABLE_SANITIZER "Specify whether to enable compiler sanitizers (address)" OFF)
endif()


###############################################################################
# Optimization / internal linking preferences
option(OCIO_USE_SIMD "Specify whether to enable SIMD CPU performance optimizations" ON)
option(OCIO_USE_OIIO_FOR_APPS "Request OIIO to build apps (ociolutimage, ocioconvert and ociodisplay), the default uses OpenEXR." OFF)


if (NOT APPLE)
    if("${CMAKE_GENERATOR_PLATFORM}" MATCHES "(ARM64|arm64)" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "ARM64")
        set(OCIO_ARCH_X86 0)
        set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE ON)
        set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_AVX OFF)
        set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_F16C OFF)
    elseif ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "(AMD64|IA64|EM64T|x86_64|X86|i386|i686)")
        # Intel-based architecture (not APPLE)
        if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "(X86|i386|i686)")
            set(OCIO_ARCH_X86_32 1)
        endif()
        set(OCIO_ARCH_X86 1)
        set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE ON)
        set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_AVX ON)
        set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_F16C ON)
    else()
        set(OCIO_ARCH_X86 0)
        set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE OFF)
        set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_AVX OFF)
        set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_F16C OFF)
    endif()
elseif (APPLE)
    # Multiple possible scenarios:
    # A. Building on an Apple arm64 architecture for the native architecture (arm64).
    # B. Building on an Apple arm64 architecture for x86_64.
    # C. Building on an Apple arm64 architecture for universal binaries (x86_64 + arm64).
    #
    # D. Building on an Apple Intel architecture for the native architecture (x86_64).
    # E. Building on an Apple Intel architecture for arm64.
    # F. Building on an Apple Intel architecture for universal binaries (x86_64 + arm64).

    # Check if we are NOT cross-compiling to another Apple architecture.
    if (NOT DEFINED CMAKE_OSX_ARCHITECTURES OR CMAKE_OSX_ARCHITECTURES STREQUAL "")
        # Building for the native architectures.
        if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "x86_64")
            # Apple Intel-based computer or Rosetta. (covers case D)

            set(OCIO_ARCH_X86 1)
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE ON)
            # The AVX and F16C will be compiled, but it won't used at runtime by Rosetta.
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_AVX ON)
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_F16C ON)
        elseif ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "arm64")
            # Apple arm64. (covers case A)

            set(OCIO_ARCH_X86 0)
            # OCIO translates the SSE instructions into ARM Neon with the library SSE2NEON.
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE ON)
            # OCIO does not translate AVX to ARM Neon.
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_AVX OFF)
            # Turn F16C off since they are x86_64 instructions.
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_F16C OFF)
        endif()
    else()
        # Cross-compiling to another Apple architecture (x86_64, arm64, or both (universal))
        if ("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "x86_64")
            # Compiling for x86_64 regardless of native architecture.
            # Covers case B assuming CMAKE_SYSTEM_PROCESSOR is arm64.
            # Covers case D if CMAKE_OSX_ARCHITECTURES is set to x86_64 on x86_64 architecture.

            set(OCIO_ARCH_X86 1)
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE ON)
            # The AVX and F16C will be compiled, but they won't used at runtime by Rosetta.
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_AVX ON)
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_F16C ON)
        elseif ("${CMAKE_OSX_ARCHITECTURES}" STREQUAL "arm64")
            # Compiling for arm64 regardless of native architecture.
            # Covers case E assuming CMAKE_SYSTEM_PROCESSOR is x86_64.
            # Covers case A if CMAKE_OSX_ARCHITECTURES is set arm64 on arm64 architecture.

            set(OCIO_ARCH_X86 0)
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE ON)
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_AVX OFF)
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_F16C OFF)
        elseif ("${CMAKE_OSX_ARCHITECTURES}" MATCHES "(arm64;x86_64|x86_64;arm64)")
            # Universal build (covers case C and F).

            set(OCIO_ARCH_X86 1)
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE ON)
            # The AVX and F16C will be compiled, but they won't used at runtime by Rosetta.
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_AVX ON)
            set(OCIO_BUILD_ENABLE_OPTIMIZATIONS_F16C ON)
        endif()
    endif()
endif()

option(OCIO_USE_SSE2 "Specify whether to enable SSE2 CPU performance optimizations" ${OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE})
option(OCIO_USE_SSE3 "Specify whether to enable SSE3 CPU performance optimizations" ${OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE})
option(OCIO_USE_SSSE3 "Specify whether to enable SSSE3 CPU performance optimizations" ${OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE})
option(OCIO_USE_SSE4 "Specify whether to enable SSE4 CPU performance optimizations" ${OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE})
option(OCIO_USE_SSE42 "Specify whether to enable SSE4.2 CPU performance optimizations" ${OCIO_BUILD_ENABLE_OPTIMIZATIONS_SSE})
option(OCIO_USE_AVX "Specify whether to enable AVX CPU performance optimizations" ${OCIO_BUILD_ENABLE_OPTIMIZATIONS_AVX})
option(OCIO_USE_AVX2 "Specify whether to enable AVX2 CPU performance optimizations" ${OCIO_BUILD_ENABLE_OPTIMIZATIONS_AVX})
option(OCIO_USE_AVX512 "Specify whether to enable AVX512 CPU performance optimizations" ${OCIO_BUILD_ENABLE_OPTIMIZATIONS_AVX})
option(OCIO_USE_F16C  "Specify whether to enable F16C CPU performance optimizations" ${OCIO_BUILD_ENABLE_OPTIMIZATIONS_F16C})

if (APPLE OR WIN32)
   # TODO: Revisit whether that option is necessary.
   option(OCIO_USE_SSE2NEON "Specify whether to enable SSE CPU performance optimizations using SSE2NEON for Apple ARM architecture" ON)
   mark_as_advanced(OCIO_USE_SSE2NEON)
endif()


###############################################################################
# GPU configuration
message(STATUS "")
message(STATUS "Checking for GPU configuration...")
include(CheckSupportGL)


###############################################################################
# Check for ARM neon here because we need to know if ARM NEON is supported
# quickly. Once we know that ARM NEON is supported, we can add sse2neon library
# to the build.

if(OCIO_USE_SIMD)
    include(CheckSupportARMNeon)

    if (NOT COMPILER_SUPPORTS_ARM_NEON)
        # Force it to OFF since ARM Neon is not supported.
        set(OCIO_USE_SSE2NEON OFF)
    endif()
endif()


###############################################################################
# Add sse2neon to the build if ARM NEON intrinsics are supported.

if(OCIO_USE_SIMD AND OCIO_USE_SSE2NEON AND COMPILER_SUPPORTS_ARM_NEON)
    # Install sse2neon. Please note that sse2neon is downloaded during the configure step as it is
    # needed for CompilerFlags.cmake and CheckSupportSSEUsingSSE2NEON.cmake.

    # Sse2neon is not treated like an imported target. The logic to find sse2neon is here because 
    # a find module is not suitable for sse2neon's use case.
    find_path(sse2neon_INCLUDE_DIR
        NAMES
            sse2neon.h
        HINTS
            ${sse2neon_ROOT}
        PATH_SUFFIXES
            sse2neon
            include
            sse2neon/include
    )

    # As per instructions on sse2neon's GitHub page, the following compiler flags should be used:
    # "-march=armv8-a+fp+simd+crypto+crc". These flags are required for some ARM platforms that do
    # not enable floating point calculations or SIMD instructions by default. However, for ARM64
    # (Apple ARM platform) and x86_64 platforms, these features are already enabled by default.
    # Therefore, no additional compiler flags are needed.
    if (NOT sse2neon_INCLUDE_DIR)
        include(Installsse2neon)
    else()
        # Any changes to the following lines must be replicated in Installsse2neon.cmake as well.
        # Create a target for sse2neon (non-imported)
        add_library(sse2neon INTERFACE)
        # Add the include directories to the target.
        target_include_directories(sse2neon INTERFACE "${sse2neon_INCLUDE_DIR}")
        if(NOT MSVC)
            # Ignore the warnings coming from sse2neon.h as they are false positives.
            target_compile_options(sse2neon INTERFACE -Wno-unused-parameter)
        endif()
    endif()
endif()


###############################################################################
# Define compilation and link flags

include(CompilerFlags)

###############################################################################
# Include utility functions for setting debugging environments

include(DebuggerEnvironment)

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

if (NOT BUILD_SHARED_LIBS AND NOT OCIO_INSTALL_EXT_PACKAGES STREQUAL NONE)
   message(STATUS "Note that building the static version of OpenColorIO does not embed the dependencies\  
   into the library file. The needed dependencies must be linked to the consumer
   application or shared library that uses static OpenColorIO.

   The following mandatory dependencies MUST be linked to the consumer application or shared library:
   expat, yaml-cpp, Imath, pystring, minizip-ng and ZLIB")
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

configure_file(${PROJECT_SOURCE_DIR}/share/ocio/${OCIO_SETUP_NAME}.in
    ${CMAKE_CURRENT_BINARY_DIR}/share/ocio/${OCIO_SETUP_NAME} @ONLY)

INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/share/ocio/${OCIO_SETUP_NAME} DESTINATION ${CMAKE_INSTALL_DATADIR}/ocio/)


###############################################################################
# Generate OpenColorIO-config.cmake
# Targets exported to ${PROJECT_NAME}_EXPORTED_TARGETS will be available for
# import using find_package(OpenColorIO).

include(CMakePackageConfigHelpers)

set(OCIO_TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets.cmake")
set(OCIO_VERSION_CONFIG "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake")
set(OCIO_PROJECT_CONFIG "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
set(OCIO_CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")
set(OCIO_CUSTOM_FIND_MODULE_DIR "${CMAKE_INSTALL_PREFIX}/share/OpenColorIO/cmake/modules")
set(OCIO_CUSTOM_MACROS_MODULE_DIR "${CMAKE_INSTALL_PREFIX}/share/OpenColorIO/cmake/macros")

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

if (NOT BUILD_SHARED_LIBS)
    # Install custom macros used in the find modules.
    install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/share/cmake/macros/VersionUtils.cmake
        DESTINATION ${OCIO_CUSTOM_MACROS_MODULE_DIR}
    )

    # Install custom Find modules.
    install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/share/cmake/modules/Findexpat.cmake
        ${CMAKE_CURRENT_LIST_DIR}/share/cmake/modules/FindImath.cmake
        ${CMAKE_CURRENT_LIST_DIR}/share/cmake/modules/Findpystring.cmake
        ${CMAKE_CURRENT_LIST_DIR}/share/cmake/modules/Findminizip-ng.cmake
        ${CMAKE_CURRENT_LIST_DIR}/share/cmake/modules/Findyaml-cpp.cmake
        DESTINATION ${OCIO_CUSTOM_FIND_MODULE_DIR}
    )
endif()

install(
    FILES "${OCIO_PROJECT_CONFIG}" "${OCIO_VERSION_CONFIG}"
    DESTINATION "${OCIO_CONFIG_INSTALL_DIR}"
)
