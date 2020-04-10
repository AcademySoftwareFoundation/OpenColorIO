# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    ${CMAKE_SOURCE_DIR}/tests/cpu/UnitTestLogUtils.cpp
    CategoryHelpers_tests.cpp
    ColorSpaceHelpers_tests.cpp
    DisplayViewHelpers_tests.cpp
    MixingHelpers_tests.cpp
    UnitTestMain.cpp
)

add_executable(test_apphelpers_exec ${SOURCES})

target_include_directories(test_apphelpers_exec
    PRIVATE
        "${CMAKE_SOURCE_DIR}/tests/cpu"
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(test_apphelpers_exec
        PRIVATE
            OpenColorIO_SKIP_IMPORTS
    )
endif()

set_target_properties(test_apphelpers_exec
    PROPERTIES
        COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}"
)

target_link_libraries(test_apphelpers_exec
    PRIVATE
        apphelpers
        OpenColorIO
        testutils
        unittest_data
)

add_test(test_apphelpers test_apphelpers_exec)

# Note: To avoid changing PATH from outside the cmake files.
if(MSVC AND BUILD_SHARED_LIBS)

    set(BUILD_TYPE "")
    if (MSVC_IDE)
        # Note: By default Microsoft Visual Studio editor happens the build type to the build directory.
        set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
    endif()

    set_tests_properties(test_apphelpers 
            PROPERTIES ENVIRONMENT 
                PATH=${CMAKE_BINARY_DIR}/src/OpenColorIO/${BUILD_TYPE} )

endif()
