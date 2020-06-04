# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

add_subdirectory(data)

if(OCIO_BUILD_TESTS)
    # Test order is from low-level (i.e. no dependencies)
    # to high-level layers.
    add_subdirectory(testutils)
    add_subdirectory(utils)
	add_subdirectory(cpu)
	add_subdirectory(apphelpers)
endif()

if(OCIO_BUILD_GPU_TESTS)
	add_subdirectory(gpu)
endif()

if(OCIO_BUILD_JAVA)
	add_subdirectory(java)
endif()

#if(OCIO_BUILD_PYTHON)
	add_subdirectory(python)
#endif()
