find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)

set(SOURCES
	CDLOps_test.cpp
	Config_test.cpp
	ExpOps_Test.cpp
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

set_target_properties(test_gpu_exec PROPERTIES 
	COMPILE_FLAGS "${PLATFORM_COMPILE_FLAGS}")

target_link_libraries(test_gpu_exec
	PRIVATE
		OpenColorIO
		openglbuilder
		unittest_data
		${GLEW_LIBRARIES}
		${GLUT_LIBRARIES}
)

add_test(test_gpu test_gpu_exec)
