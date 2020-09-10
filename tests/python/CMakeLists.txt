# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

find_package(Python REQUIRED COMPONENTS Interpreter)

set(BUILD_TYPE "")
if (MSVC_IDE)
    # Note: By default Microsoft Visual Studio editor happens the build type to the build directory.
    set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()


set(SSE_ENABLED False)
if(OCIO_USE_SSE)
    set(SSE_ENABLED True)
endif()

add_test(test_python ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/OpenColorIOTestSuite.py ${CMAKE_BINARY_DIR} ${SSE_ENABLED} ${BUILD_TYPE})

