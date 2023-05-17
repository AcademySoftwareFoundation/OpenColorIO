# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(ExternalProject)

set(UNITTEST_DEST_DIR "${PROJECT_BINARY_DIR}/testdata")

# Need to add this superfluous file command to allow imported target to depend on ExternalProject
file(MAKE_DIRECTORY ${UNITTEST_DEST_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/files/ DESTINATION ${UNITTEST_DEST_DIR})
add_library(unittest_data INTERFACE IMPORTED GLOBAL)
set_property(TARGET unittest_data PROPERTY INTERFACE_COMPILE_DEFINITIONS "OCIO_UNIT_TEST_FILES_DIR=${UNITTEST_DEST_DIR}")
