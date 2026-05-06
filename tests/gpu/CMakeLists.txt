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
	GPUUnitTest.cpp
	GradingPrimaryOp_test.cpp
	GradingRGBCurveOp_test.cpp
	GradingHueCurveOp_test.cpp
	GradingToneOp_test.cpp
	LogOp_test.cpp
	Lut1DOp_test.cpp
	Lut3DOp_test.cpp
	MatrixOp_test.cpp
	RangeOp_test.cpp
)

add_executable(test_gpu_exec ${SOURCES})

set_target_properties(test_gpu_exec PROPERTIES 
	COMPILE_OPTIONS "${PLATFORM_COMPILE_OPTIONS}"
	LINK_OPTIONS "${PLATFORM_LINK_OPTIONS}"
)

target_link_libraries(test_gpu_exec
	PRIVATE
		OpenColorIO
		oglapphelpers
		pystring::pystring
		unittest_data
		utils::strings
		testutils
)

add_test(NAME test_gpu COMMAND test_gpu_exec)
if(APPLE)
	add_test(NAME test_metal COMMAND test_gpu_exec -metal)
endif()

# Note: To avoid changing PATH from outside the cmake files.
if(MSVC AND BUILD_SHARED_LIBS)
	# Build time list of runtime dll dirs for the exe target.
	set(dll_dirs_expr "$<JOIN:$<TARGET_RUNTIME_DLL_DIRS:test_gpu_exec>,;>")

	# Add folders for glut and glew DLLs.
	set(extra_dirs "${GLUT_INCLUDE_DIR}/../bin\\;${GLEW_INCLUDE_DIRS}/../bin\\")

	# Tell CTest to use this PATH while launching test_gpu_exec
	set_tests_properties(test_gpu PROPERTIES
		ENVIRONMENT "PATH=${dll_dirs_expr}\\;${extra_dirs}"
	)

	# Also set the debugger environment so that you can launch 
	# test_gpu_exec directly within Visual Studio.
	set_property(TARGET test_gpu_exec PROPERTY
		VS_DEBUGGER_ENVIRONMENT "PATH=${dll_dirs_expr};${extra_dirs};%PATH%"
	)
endif()
