# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

find_package(OpenGL QUIET)
if(NOT OpenGL_FOUND)
	message(WARNING "OpenGL not found. Skipping the GPU unit tests.")
	return()
endif()

if(NOT APPLE)
	find_package(GLEW QUIET)
	if(NOT GLEW_FOUND)
		message(WARNING "GLEW not found. Skipping the GPU unit tests.")
		return()
	endif()
endif()

find_package(GLUT QUIET)
if(NOT GLUT_FOUND)
	message(WARNING "GLUT not found. Skipping the GPU unit tests.")
	return()
endif()

set(SOURCES
	CDLOps_test.cpp
	Config_test.cpp
	ECOps_test.cpp
	ExpOps_test.cpp
	FixedFunctionOps_test.cpp
	GPUHelpers.cpp
	GPUUnitTest.cpp
	LogOps_test.cpp
	Lut1DOp_test.cpp
	Lut3DOp_test.cpp
	MatrixOps_test.cpp
	RangeOps_test.cpp
)

add_executable(test_gpu_exec ${SOURCES})
target_include_directories(test_gpu_exec
	PRIVATE
		${OPENGL_INCLUDE_DIR}
		${GLEW_INCLUDE_DIRS}
		${GLUT_INCLUDE_DIR}
)

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

if(APPLE)
	# Mute the deprecated warning for some GLUT methods.
	set(PLATFORM_COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS} -Wno-deprecated-declarations")
endif()

set_target_properties(test_gpu_exec PROPERTIES 
	COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(test_gpu_exec
	PRIVATE
		OpenColorIO
		oglbuilder
		unittest_data
		${GLEW_LIBRARIES}
		${GLUT_LIBRARIES}
)

add_test(test_gpu test_gpu_exec)
