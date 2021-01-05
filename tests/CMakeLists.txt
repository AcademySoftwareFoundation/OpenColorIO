# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(OCIO_BUILD_TESTS OR OCIO_BUILD_GPU_TESTS OR OCIO_BUILD_PYTHON)
	# Copy test files if used.
	add_subdirectory(data)
endif()

if(OCIO_BUILD_TESTS)
	# Test order is from low-level (i.e. no dependencies)
	# to high-level layers.
	add_subdirectory(testutils)
	add_subdirectory(utils)
	add_subdirectory(cpu)
endif()

if(OCIO_BUILD_GPU_TESTS)
	add_subdirectory(gpu)
endif()

if(OCIO_BUILD_JAVA)
	add_subdirectory(java)
endif()

if(OCIO_BUILD_PYTHON)
	add_subdirectory(python)
endif()
