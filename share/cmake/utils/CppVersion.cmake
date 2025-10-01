# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# C++ version configuration

set(SUPPORTED_CXX_STANDARDS 17 20 23)
string(REPLACE ";" ", " SUPPORTED_CXX_STANDARDS_STR "${SUPPORTED_CXX_STANDARDS}")

if(NOT DEFINED CMAKE_CXX_STANDARD)
    message(STATUS "Setting C++ version to '17' as none was specified.")
    set(CMAKE_CXX_STANDARD 17 CACHE STRING "C++ standard to compile against")
elseif(NOT CMAKE_CXX_STANDARD IN_LIST SUPPORTED_CXX_STANDARDS)
    message(WARNING
            "CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} is unsupported. Supported standards are: ${SUPPORTED_CXX_STANDARDS_STR}.")
endif()

set_property(CACHE CMAKE_CXX_STANDARD PROPERTY STRINGS "${SUPPORTED_CXX_STANDARDS}")

include(CheckCXXCompilerFlag)

# As CheckCXXCompilerFlag implicitly uses CMAKE_CXX_FLAGS some custom flags could trigger unrelated
# warnings causing a detection failure. So, the code disables all warnings to focus on the C++
# version detection.
if(USE_MSVC)
    set(CUSTOM_CXX_FLAGS "/w")
else()
    set(CUSTOM_CXX_FLAGS "-w")
endif()

if(${CMAKE_CXX_STANDARD} EQUAL 17)
    if(USE_MSVC)
        CHECK_CXX_COMPILER_FLAG("${CUSTOM_CXX_FLAGS} -std:c++17" COMPILER_SUPPORTS_CXX17)
    else()
        CHECK_CXX_COMPILER_FLAG("${CUSTOM_CXX_FLAGS} -std=c++17" COMPILER_SUPPORTS_CXX17)
    endif()

    if(NOT COMPILER_SUPPORTS_CXX17)
        message(FATAL_ERROR 
            "The compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} has no C++17 support.")
    endif()
endif()

if(${CMAKE_CXX_STANDARD} EQUAL 20)
    if(USE_MSVC)
        CHECK_CXX_COMPILER_FLAG("${CUSTOM_CXX_FLAGS} -std:c++20" COMPILER_SUPPORTS_CXX20)
    else()
        CHECK_CXX_COMPILER_FLAG("${CUSTOM_CXX_FLAGS} -std=c++20" COMPILER_SUPPORTS_CXX20)
    endif()

    if(NOT COMPILER_SUPPORTS_CXX20)
        message(FATAL_ERROR 
            "The compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} has no C++20 support.")
    endif()
endif()

if(${CMAKE_CXX_STANDARD} EQUAL 23)
    if(USE_MSVC)
        CHECK_CXX_COMPILER_FLAG("${CUSTOM_CXX_FLAGS} -std:c++23" COMPILER_SUPPORTS_CXX23)
    else()
        CHECK_CXX_COMPILER_FLAG("${CUSTOM_CXX_FLAGS} -std=c++23" COMPILER_SUPPORTS_CXX23)
    endif()

    if(NOT COMPILER_SUPPORTS_CXX23)
        message(FATAL_ERROR 
            "The compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} has no C++23 support.")
    endif()
endif()

# Disable fallback to other C++ version if standard is not supported.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable any compiler specific C++ extensions.
set(CMAKE_CXX_EXTENSIONS OFF)
