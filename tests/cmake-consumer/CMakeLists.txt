# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(TEST_DEST_DIR "${PROJECT_BINARY_DIR}/tests/cmake-consumer-dist")

file(MAKE_DIRECTORY ${TEST_DEST_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.in ${TEST_DEST_DIR}/CMakeLists.txt COPYONLY)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/consumer.cpp DESTINATION ${TEST_DEST_DIR})
