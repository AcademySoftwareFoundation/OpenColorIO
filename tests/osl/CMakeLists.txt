# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

set(SOURCES
    CDLOp_test.cpp
    ECOp_test.cpp
    FixedFunctionOp_test.cpp
    GammaOp_test.cpp
    GradingPrimaryOp_test.cpp
    GradingRGBCurveOp_test.cpp
    GradingToneOp_test.cpp
    LogOp_test.cpp
    MatrixOp_test.cpp
    RangeOp_test.cpp
    UnitTestMain.cpp
    UnitTestOSL.cpp
)

add_executable(test_osl_exec ${SOURCES})

set_target_properties(test_osl_exec PROPERTIES 
    COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
    LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)

# Because some OpenImageIO types are present in the OSL public API that dependency is mandatory;
# however, the unit test framework itself does not have any OpenImageIO dependency.

target_link_libraries(test_osl_exec
    PRIVATE
        OpenColorIO
        osl::osl
        OpenImageIO::OpenImageIO
)

add_test(NAME test_osl COMMAND test_osl_exec)

list(APPEND ENVS "OSL_SHADERS_DIR=${OSL_SHADERS_DIR}")
list(APPEND ENVS "TMP_SHADERS_DIR=${CMAKE_CURRENT_BINARY_DIR}")

set_tests_properties(test_osl PROPERTIES ENVIRONMENT "${ENVS}")

# Note: To avoid changing PATH from outside the cmake files.
if(MSVC AND BUILD_SHARED_LIBS)

    if (MSVC_IDE)
        # Note: By default Microsoft Visual Studio editor happens the build type to the build directory.
        set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
    endif()

    set(NEW_PATH "${PROJECT_BINARY_DIR}/src/OpenColorIO/${BUILD_TYPE}")
    set(NEW_PATH "${NEW_PATH}\\\;${GLUT_INCLUDE_DIR}/../bin")
    set(NEW_PATH "${NEW_PATH}\\\;${GLEW_INCLUDE_DIRS}/../bin")

    set_tests_properties(test_osl PROPERTIES ENVIRONMENT PATH=${NEW_PATH})

endif()
