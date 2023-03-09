# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    UnitTest.cpp
)

add_library(testutils STATIC ${SOURCES})

set_target_properties(testutils PROPERTIES
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)

target_include_directories(testutils
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(testutils
    PRIVATE
        apputils
        utils::strings
)
