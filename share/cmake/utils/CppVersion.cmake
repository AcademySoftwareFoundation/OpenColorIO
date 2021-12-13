# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.


###############################################################################
# C++ version configuration

set(SUPPORTED_CXX_STANDARDS 11 14 17)
string(REPLACE ";" ", " SUPPORTED_CXX_STANDARDS_STR "${SUPPORTED_CXX_STANDARDS}")

if(NOT DEFINED CMAKE_CXX_STANDARD)
    message(STATUS "Setting C++ version to '14' as none was specified.")
    set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ standard to compile against")
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

if(${CMAKE_CXX_STANDARD} EQUAL 11)
    if(USE_MSVC)
        CHECK_CXX_COMPILER_FLAG("${CUSTOM_CXX_FLAGS} -std:c++11" COMPILER_SUPPORTS_CXX11)
    else()
        CHECK_CXX_COMPILER_FLAG("${CUSTOM_CXX_FLAGS} -std=c++11" COMPILER_SUPPORTS_CXX11)
    endif()

    if(NOT COMPILER_SUPPORTS_CXX11)
        message(STATUS 
            "The compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} has no C++11 only support. Use C++14.")
        set(CMAKE_CXX_STANDARD 14)
    endif()
endif()

if(${CMAKE_CXX_STANDARD} EQUAL 14)
    if(USE_MSVC)
        CHECK_CXX_COMPILER_FLAG("${CUSTOM_CXX_FLAGS} -std:c++14" COMPILER_SUPPORTS_CXX14)
    else()
        CHECK_CXX_COMPILER_FLAG("${CUSTOM_CXX_FLAGS} -std=c++14" COMPILER_SUPPORTS_CXX14)
    endif()

    if(NOT COMPILER_SUPPORTS_CXX14)
        message(STATUS 
            "The compiler ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} has no C++14 only support. Use C++17.")
        set(CMAKE_CXX_STANDARD 17)
    endif()
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


# Disable fallback to other C++ version if standard is not supported.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disable any compiler specific C++ extensions.
set(CMAKE_CXX_EXTENSIONS OFF)
