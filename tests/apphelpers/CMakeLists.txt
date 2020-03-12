# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    ${CMAKE_SOURCE_DIR}/tests/cpu/UnitTest.cpp
    ${CMAKE_SOURCE_DIR}/tests/cpu/UnitTestLogUtils.cpp
    CategoryHelpers_tests.cpp
    ColorSpaceHelpers_tests.cpp
    DisplayViewHelpers_tests.cpp
    MixingHelpers_tests.cpp
)

add_executable(test_apphelpers_exec ${SOURCES})

target_include_directories(test_apphelpers_exec
    PRIVATE
        "${CMAKE_SOURCE_DIR}/tests/cpu"
        "${CMAKE_SOURCE_DIR}/src/utils/apphelpers"
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(test_apphelpers_exec
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

target_compile_definitions(test_apphelpers_exec
    PRIVATE
        OCIO_UNIT_TEST
)

set_target_properties(test_apphelpers_exec
    PROPERTIES
        COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}"
)

target_link_libraries(test_apphelpers_exec
    PUBLIC
        public_api

    PRIVATE
        apphelpers
        OpenColorIO
        unittest_data
)

add_test(test_apphelpers test_apphelpers_exec)
