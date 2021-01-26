# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# Define which compiler is used.

set(USE_MSVC  OFF)
set(USE_CLANG OFF)
set(USE_GCC   OFF)

if(MSVC)
    set(USE_MSVC ON)
elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
    set(USE_CLANG ON)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(USE_GCC ON)
else()
    message(FATAL_ERROR "No compiler support for '${CMAKE_CXX_COMPILER_ID}' compiler.")
endif()


# An advanced variable will not be displayed in any of the cmake GUIs

mark_as_advanced(USE_MSVC)
mark_as_advanced(USE_CLANG)
mark_as_advanced(USE_GCC)
