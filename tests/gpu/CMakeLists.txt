# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(NOT OCIO_GL_ENABLED)
    message(WARNING "GL component missing. Skipping the GPU unit tests.")
    return()
endif()

set(SOURCES
	CDLOp_test.cpp
	Config_test.cpp
	ECOp_test.cpp
	FixedFunctionOp_test.cpp
	GammaOp_test.cpp
	GPUHelpers.cpp
	GPUUnitTest.cpp
	LogOp_test.cpp
	Lut1DOp_test.cpp
	Lut3DOp_test.cpp
	MatrixOp_test.cpp
	RangeOp_test.cpp
)

add_executable(test_gpu_exec ${SOURCES})

if(NOT BUILD_SHARED_LIBS)
	target_compile_definitions(test_gpu_exec
		PRIVATE
			OpenColorIO_SKIP_IMPORTS
	)
endif()

if(OCIO_USE_SSE)
	target_compile_definitions(test_gpu_exec
		PRIVATE
			USE_SSE
	)
endif(OCIO_USE_SSE)

set_target_properties(test_gpu_exec PROPERTIES 
	COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(test_gpu_exec
	PRIVATE
		OpenColorIO
		oglapphelpers
		unittest_data
)

add_test(test_gpu test_gpu_exec)

# Note: To avoid changing PATH from outside the cmake files.
if(MSVC AND BUILD_SHARED_LIBS)

    if (MSVC_IDE)
        # Note: By default Microsoft Visual Studio editor happens the build type to the build directory.
        set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
    endif()

    set(NEW_PATH "${CMAKE_BINARY_DIR}/src/OpenColorIO/${BUILD_TYPE}")
    set(NEW_PATH "${NEW_PATH}\\\;${GLUT_INCLUDE_DIR}/../bin")
    set(NEW_PATH "${NEW_PATH}\\\;${GLEW_INCLUDE_DIRS}/../bin")

    set_tests_properties(test_gpu PROPERTIES ENVIRONMENT PATH=${NEW_PATH})

endif()
