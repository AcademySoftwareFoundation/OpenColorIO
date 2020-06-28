# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

cmake_minimum_required(VERSION 3.12)

project(OpenColorIO 
	VERSION 2.0.0
	LANGUAGES CXX C)

# "dev" for master or "" for any official release
set(OpenColorIO_VERSION_RELEASE_TYPE "dev")

enable_testing()

set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/share/cmake/macros
    ${CMAKE_SOURCE_DIR}/share/cmake/modules
)
set(CMAKE_WARN_DEPRECATED ON)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

###############################################################################
# C++ version configuration

set(SUPPORTED_CXX_STANDARDS 11 14)
string(REPLACE ";" ", " SUPPORTED_CXX_STANDARDS_STR "${SUPPORTED_CXX_STANDARDS}")

if(CMAKE_CXX_STANDARD AND NOT CMAKE_CXX_STANDARD IN_LIST SUPPORTED_CXX_STANDARDS)
    message(FATAL_ERROR " CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD} is unsupported. Supported standards are: ${SUPPORTED_CXX_STANDARDS_STR}.")
elseif(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()

include(CheckCXXCompilerFlag)

if(MSVC)
	CHECK_CXX_COMPILER_FLAG("-std:c++11" COMPILER_SUPPORTS_CXX11)
	CHECK_CXX_COMPILER_FLAG("-std:c++14" COMPILER_SUPPORTS_CXX14)
else()
	CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
	CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
endif()

if(NOT COMPILER_SUPPORTS_CXX11 AND ${CMAKE_CXX_STANDARD} EQUAL 11)
	message(STATUS "The compiler ${CMAKE_CXX_COMPILER_ID} has no C++11 only support. Use C++14.")
	set(CMAKE_CXX_STANDARD 14)
endif()

if(NOT COMPILER_SUPPORTS_CXX14 AND ${CMAKE_CXX_STANDARD} EQUAL 14)
	message(ERROR "The compiler ${CMAKE_CXX_COMPILER_ID} has no C++14 only support.")
endif()


# Disable fallback to other C++ version if standard is not supported.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Disable any compiler specific C++ extensions.
set(CMAKE_CXX_EXTENSIONS OFF)

set(_BUILD_TYPE_DEBUG OFF)
if(CMAKE_BUILD_TYPE MATCHES "[Dd][Ee][Bb][Uu][Gg]")
    set(_BUILD_TYPE_DEBUG ON)
endif()
set(BUILD_TYPE_DEBUG ${_BUILD_TYPE_DEBUG} CACHE BOOL "Case-insensitive CMAKE_BUILD_TYPE Debug equality flag.")

###############################################################################
# Components to build

option(BUILD_SHARED_LIBS "Set to OFF to build static libraries" ON)
option(OCIO_BUILD_APPS "Set to OFF to disable command-line apps" ON)
option(OCIO_BUILD_NUKE "Specify whether to build nuke plugins" OFF)
option(OCIO_BUILD_DOCS "Specify whether to build documentation" OFF)
option(OCIO_BUILD_TESTS "Specify whether to build unittests" ON)
option(OCIO_BUILD_GPU_TESTS "Specify whether to build gpu unittests" ON)

option(OCIO_BUILD_PYTHON "Specify whether to build python bindings" ON)
option(OCIO_BUILD_JAVA "Specify whether to build java bindings" OFF)

option(OCIO_WARNING_AS_ERROR "Set build error level for CI testing" OFF)

###############################################################################
# GPU configuration

include(PackageUtils)

if(OCIO_BUILD_GPU_TESTS OR OCIO_BUILD_APPS)
	set(OCIO_GL_ENABLED ON)
	
	find_package(OpenGL)
	if(NOT OpenGL_FOUND)
		package_root_message(OpenGL)
        set(OCIO_GL_ENABLED OFF)
	endif()
	
	if(NOT APPLE)
        find_package(GLEW)
        if(NOT GLEW_FOUND)
			package_root_message(GLEW)
            set(OCIO_GL_ENABLED OFF)
        endif()
	endif()
	
    find_package(GLUT)
    if(NOT GLUT_FOUND)
		package_root_message(GLUT)
        set(OCIO_GL_ENABLED OFF)
	endif()
	
endif()

###############################################################################
# Optimization / internal linking preferences

option(OCIO_USE_SSE "Specify whether to enable SSE CPU performance optimizations" ON)
option(OCIO_INLINES_HIDDEN "Specify whether to build with -fvisibility-inlines-hidden" ${UNIX})

###############################################################################
# Supplemental builtins

# Add supplemental built-in transformations such as camera related one.
option(OCIO_ADD_EXTRA_BUILTINS "Specify whether to add supplemental built-in transforms" ON)

###############################################################################
# Platform & Compiler dependent compilation flags

set(PLATFORM_COMPILE_FLAGS "")

if(MSVC)
	# Because our Exceptions derive from std::runtime_error we can safely disable warning 4275
	# /we4062 Enables warning in switch when an enumeration value is not explicitly handled.
	set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /EHsc /wd4275 /DWIN32 /we4062")

else()
    if(APPLE AND OCIO_GL_ENABLED)
        # Mute the deprecated warning for some GLUT methods.
        set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Wno-deprecated-declarations")
    endif()

	# -Wswitch-enum Enables warning in switch when an enumeration value is not explicitly handled.
	set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Wall -Wswitch-enum")

	# TODO: OCIO being under active development, unused functions are fine for now.
	set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Wno-unused-function")

	# TODO: A C++17 specific warning when compiling in C++11 !!
	if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		# Use of 'register' specifier must be removed for C++17 support.
		set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Wno-deprecated-register")
	endif()
endif()

if(OCIO_WARNING_AS_ERROR)
	if(MSVC)
		set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} /WX")
	else()
		set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Werror")
	endif()
endif()

include(CheckSSEFeatures)
if(NOT HAVE_SSE2)
	message(STATUS "Disabling SSE optimizations, as the target doesn't support them")
	set(OCIO_USE_SSE OFF)
endif(NOT HAVE_SSE2)

###############################################################################
# External linking options

set(OCIO_INSTALL_EXT_PACKAGES "MISSING" CACHE STRING "Set the condition for Installing external dependencies")
set_property(CACHE OCIO_INSTALL_EXT_PACKAGES PROPERTY STRINGS "NONE" "MISSING" "ALL")

###############################################################################
# Versioning

if(NOT SOVERSION)
	set(SOVERSION "${OpenColorIO_VERSION_MAJOR}.${OpenColorIO_VERSION_MINOR}" CACHE STRING "Set the SO version in the SO name of the output library")
endif()

###############################################################################
# Namespace

if(NOT OCIO_NAMESPACE)
	set(OCIO_NAMESPACE OpenColorIO CACHE STRING "Specify the master OCIO C++ namespace: Options include OpenColorIO OpenColorIO_<YOURFACILITY> etc.")
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
	message(FATAL_ERROR " Deprecated options OCIO_BUILD_SHARED and OCIO_BUILD_STATIC cannot both be used at once")
endif()

if(OCIO_BUILD_SHARED)
	message(DEPRECATION " Option OCIO_BUILD_SHARED is deprecated. Please use the cmake standard BUILD_SHARED_LIBS=ON (default ON)")
	if(NOT BUILD_SHARED_LIBS)
		set(BUILD_SHARED_LIBS ON)
	endif()
endif()

if(OCIO_BUILD_STATIC)
	message(DEPRECATION " Option OCIO_BUILD_STATIC is deprecated. Please use the cmake standard BUILD_SHARED_LIBS=OFF (default ON)")
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
add_subdirectory(tests)
add_subdirectory(include)
add_subdirectory(src)
if(OCIO_BUILD_DOCS)
	add_subdirectory(docs)
endif()
add_subdirectory(vendor)
