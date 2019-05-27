find_package(OpenGL REQUIRED)
if(NOT APPLE)
	find_package(GLEW REQUIRED)
endif()
find_package(GLUT REQUIRED)

set(SOURCES
	CDLOps_test.cpp
	Config_test.cpp
	ECOps_test.cpp
	ExpOps_Test.cpp
	FixedFunctionOps_test.cpp
	GPUHelpers.cpp
	GPUUnitTest.cpp
	LogOps_Test.cpp
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
