# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(ExternalProject)

set(SOURCES
    UnitTestMain.cpp
    StringUtils_tests.cpp
)

add_executable(test_utils_exec ${SOURCES})

target_link_libraries(test_utils_exec
    PRIVATE
        utils::strings
        testutils
)

set_target_properties(test_utils_exec PROPERTIES
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)

add_test(NAME test_utils_exec COMMAND test_utils_exec)
