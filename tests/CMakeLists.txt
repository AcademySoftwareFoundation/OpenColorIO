# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

add_subdirectory(data)
if(OCIO_BUILD_TESTS)
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
