# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

include(ExternalProject)

function(add_ext_test NAME SOURCES PRIVATE_INCLUDES)
    set(TEST_BINARY "test_${NAME}_exec")
    set(TEST_NAME "test_${NAME}")

    add_executable(${TEST_BINARY} ${SOURCES})

    target_compile_definitions(${TEST_BINARY}
        PRIVATE
            OCIO_UNIT_TEST
            OpenColorIO_SKIP_IMPORTS

    )

    if(PRIVATE_INCLUDES)
        target_include_directories(${TEST_BINARY}
            PRIVATE
                "${CMAKE_SOURCE_DIR}/ext/utils"
                "${CMAKE_SOURCE_DIR}/tests/cpu"
        )
    endif(PRIVATE_INCLUDES)

    set_target_properties(${TEST_BINARY} PROPERTIES
        COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

    add_test(${TEST_NAME} ${TEST_BINARY})
endfunction(add_ext_test)

set(SOURCES
    ${CMAKE_SOURCE_DIR}/tests/cpu/UnitTest.cpp
)

set(TESTS
    StringUtils_tests.cpp
)

list(APPEND SOURCES ${TESTS})

add_ext_test(ext "${SOURCES}" TRUE)
