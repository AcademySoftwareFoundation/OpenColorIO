# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# C++ version configuration

set(SUPPORTED_CXX_STANDARDS 11 14 17)
string(REPLACE ";" ", " SUPPORTED_CXX_STANDARDS_STR "${SUPPORTED_CXX_STANDARDS}")

if(NOT DEFINED CMAKE_CXX_STANDARD)
    message(STATUS "Setting C++ version to '11' as none was specified.")
    set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ standard to compile against")
elseif(NOT CMAKE_CXX_STANDARD IN_LIST SUPPORTED_CXX_STANDARDS)
    message(FATAL_ERROR 
            "CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} is unsupported. Supported standards are: ${SUPPORTED_CXX_STANDARDS_STR}.")
endif()

set_property(CACHE CMAKE_CXX_STANDARD PROPERTY STRINGS "${SUPPORTED_CXX_STANDARDS}")

include(CheckCXXCompilerFlag)

if(USE_MSVC)
    CHECK_CXX_COMPILER_FLAG("-std:c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std:c++14" COMPILER_SUPPORTS_CXX14)
    CHECK_CXX_COMPILER_FLAG("-std:c++17" COMPILER_SUPPORTS_CXX17)
else()
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    CHECK_CXX_COMPILER_FLAG("-std=c++14" COMPILER_SUPPORTS_CXX14)
    CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
endif()

if(NOT COMPILER_SUPPORTS_CXX11 AND ${CMAKE_CXX_STANDARD} EQUAL 11)
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER_ID} has no C++11 only support. Use C++14.")
    set(CMAKE_CXX_STANDARD 14)
endif()

if(NOT COMPILER_SUPPORTS_CXX14 AND ${CMAKE_CXX_STANDARD} EQUAL 14)
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER_ID} has no C++14 only support. Use C++17.")
    set(CMAKE_CXX_STANDARD 17)
endif()

if(NOT COMPILER_SUPPORTS_CXX17 AND ${CMAKE_CXX_STANDARD} EQUAL 17)
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER_ID} has no C++17 support.")
endif()


# Disable fallback to other C++ version if standard is not supported.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable any compiler specific C++ extensions.
set(CMAKE_CXX_EXTENSIONS OFF)
