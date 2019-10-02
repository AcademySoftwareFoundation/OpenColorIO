# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

if(OCIO_BUILD_JAVA)
	add_subdirectory(java)
endif()

if(OCIO_BUILD_PYTHON)
	add_subdirectory(python)
endif()
