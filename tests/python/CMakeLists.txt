# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(BUILD_TYPE "")
if (MSVC_IDE)
    # Note: By default Microsoft Visual Studio editor happens the build type to the build directory.
    set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()

add_test(test_python ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/OpenColorIOTestSuite.py ${PROJECT_BINARY_DIR} ${BUILD_TYPE})
