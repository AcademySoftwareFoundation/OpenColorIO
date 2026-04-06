# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(NOT OCIO_GL_ENABLED AND NOT (WIN32 AND OCIO_DIRECTX_ENABLED) AND NOT OCIO_VULKAN_ENABLED)
    message(WARNING "GL, DirectX, and Vulkan components all missing. Skipping the GPU unit tests.")
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

if(OCIO_GL_ENABLED)
    add_test(NAME test_gpu COMMAND test_gpu_exec)
endif()
if(APPLE)
	add_test(NAME test_metal COMMAND test_gpu_exec -metal)
endif()
if(WIN32 AND OCIO_DIRECTX_ENABLED)
	add_test(NAME test_dx COMMAND test_gpu_exec --dx)
endif()
if(OCIO_VULKAN_ENABLED)
	add_test(NAME test_vulkan COMMAND test_gpu_exec --vulkan)
endif()

# Copy dxcompiler.dll and dxil.dll to the test output directory.
# These are required at runtime when DXC (IDxcCompiler3) is used for SM6.0 shader compilation.
# The Redist/D3D path is the stable, version-independent redistribution location.
if(WIN32 AND OCIO_DIRECTX_ENABLED)
    find_file(DXCOMPILER_DLL
        NAMES dxcompiler.dll
        PATHS
            # Note: x64 hardcoded; update if ARM64 Windows support is needed.
            "$ENV{WindowsSdkDir}Redist/D3D/x64"
            "C:/Program Files (x86)/Windows Kits/10/Redist/D3D/x64"
        NO_DEFAULT_PATH
        DOC "Path to dxcompiler.dll from Windows SDK"
    )
    if(DXCOMPILER_DLL)
        get_filename_component(_dxc_dll_dir "${DXCOMPILER_DLL}" DIRECTORY)
        find_file(DXIL_DLL
            NAMES dxil.dll
            HINTS "${_dxc_dll_dir}"
            NO_DEFAULT_PATH
        )
        add_custom_command(TARGET test_gpu_exec POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DXCOMPILER_DLL}" "$<TARGET_FILE_DIR:test_gpu_exec>"
            COMMENT "Copying dxcompiler.dll to test output directory"
        )
        if(DXIL_DLL)
            add_custom_command(TARGET test_gpu_exec POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DXIL_DLL}" "$<TARGET_FILE_DIR:test_gpu_exec>"
                COMMENT "Copying dxil.dll to test output directory"
            )
        endif()
    else()
        message(STATUS
            "dxcompiler.dll not found in Windows SDK Redist/D3D. "
            "Add its directory to PATH before running test_dx."
        )
    endif()
endif()

# Note: To avoid changing PATH from outside the cmake files.
if(MSVC AND BUILD_SHARED_LIBS)
	# Build time list of runtime dll dirs for the exe target.
	set(dll_dirs_expr "$<JOIN:$<TARGET_RUNTIME_DLL_DIRS:test_gpu_exec>,;>")

	# Add folders for glut and glew DLLs.
	set(extra_dirs "${GLUT_INCLUDE_DIR}/../bin\\;${GLEW_INCLUDE_DIRS}/../bin\\")

	# Tell CTest to use this PATH while launching test_gpu_exec
	if(OCIO_GL_ENABLED)
		set_tests_properties(test_gpu PROPERTIES
			ENVIRONMENT "PATH=${dll_dirs_expr}\\;${extra_dirs}"
		)
	endif()
	if(WIN32 AND OCIO_DIRECTX_ENABLED)
		set_tests_properties(test_dx PROPERTIES
			ENVIRONMENT "PATH=${dll_dirs_expr}\\;${extra_dirs}"
		)
	endif()

	# Also set the debugger environment so that you can launch 
	# test_gpu_exec directly within Visual Studio.
	set_property(TARGET test_gpu_exec PROPERTY
		VS_DEBUGGER_ENVIRONMENT "PATH=${dll_dirs_expr};${extra_dirs};%PATH%"
	)
endif()
