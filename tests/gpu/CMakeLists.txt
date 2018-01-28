find_package(GLUT REQUIRED)

set(SOURCES
	Config_test.cpp
	ExpOps_Test.cpp
	GPUHelpers.cpp
	GPUUnitTest.cpp
	LogOps_Test.cpp
	Lut1DOp_test.cpp
	Lut3DOp_test.cpp
	MatrixOps_test.cpp
)

add_executable(test_gpu_exec ${SOURCES})
target_include_directories(test_gpu_exec
	PRIVATE
		${GLUT_INCLUDE_DIR}
)
target_link_libraries(test_gpu_exec
	PRIVATE
		OpenColorIO
		openglbuilder
		unittest_data
		${GLUT_LIBRARIES}
)

add_test(test_gpu test_gpu_exec)
